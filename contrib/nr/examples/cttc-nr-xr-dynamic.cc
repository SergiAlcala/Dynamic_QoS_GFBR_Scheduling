
#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/boolean.h"
#include "ns3/config-store-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/lte-enb-rrc.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-module.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-module.h"
#include "ns3/xr-traffic-mixer-helper.h"

#include <vector>
#include <fstream>
#include <iomanip>


#include <unistd.h>  // add at top of file
#include <sys/stat.h>


#include <fstream>
#include <map>

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("CttcNrSimpleQosSchedEdit");

static std::ofstream g_dlSchedLog;


EpsBearer::Qci
Parse5qi(const std::string& s)
{
    static const std::map<std::string, EpsBearer::Qci> map5qi = {
        {"GBR_CONV_VIDEO",      EpsBearer::GBR_CONV_VIDEO},
        {"GBR_CONV_VOICE",      EpsBearer::GBR_CONV_VOICE},
        {"NGBR_LOW_LAT_EMBB",   EpsBearer::NGBR_LOW_LAT_EMBB},
        {"NGBR_VOICE_VIDEO_GAMING", EpsBearer::NGBR_VOICE_VIDEO_GAMING},
    };

    auto it = map5qi.find(s);
    NS_ABORT_MSG_IF(it == map5qi.end(), "Unknown 5QI: " << s);

    return it->second;
}

void
WriteBytesSent(Ptr<TrafficGenerator> trafficGenerator,
               uint64_t* previousBytesSent,
               uint64_t* previousWindowBytesSent,
               enum NrXrConfig NrXrConfig,
               std::ofstream* outFileTx)
{
    uint64_t totalBytesSent = trafficGenerator->GetTotalBytes();
    (*outFileTx) << "\n"
                 << Simulator::Now().GetMilliSeconds() << "\t" << *previousWindowBytesSent
                 << std::endl;
    (*outFileTx) << "\n"
                 << Simulator::Now().GetMilliSeconds() << "\t"
                 << totalBytesSent - *previousBytesSent << std::endl;

    *previousWindowBytesSent = totalBytesSent - *previousBytesSent;
    *previousBytesSent = totalBytesSent;
};

void
WriteBytesReceived(Ptr<PacketSink> packetSink, uint64_t* previousBytesReceived)
{
    uint64_t totalBytesReceived = packetSink->GetTotalRx();
    *previousBytesReceived = totalBytesReceived;
};

void
ConfigureXrApp(NodeContainer& ueContainer,
               uint32_t i,
               Ipv4InterfaceContainer& ueIpIface,
               enum NrXrConfig config,
               uint16_t port,
               std::string transportProtocol,
               NodeContainer& remoteHostContainer,
               NetDeviceContainer& ueNetDev,
               Ptr<NrHelper> nrHelper,
               EpsBearer& bearer,
               Ptr<EpcTft> tft,
               bool isMx1,
               std::vector<Ptr<EpcTft>>& tfts,
               ApplicationContainer& serverApps,
               ApplicationContainer& clientApps,
               ApplicationContainer& pingApps)
{
    XrTrafficMixerHelper trafficMixerHelper;
    Ipv4Address ipAddress = ueIpIface.GetAddress(i, 0);
    trafficMixerHelper.ConfigureXr(config);
    auto it = XrPreconfig.find(config);

    std::vector<Address> addresses;
    std::vector<InetSocketAddress> localAddresses;
    for (uint j = 0; j < it->second.size(); j++)
    {
        addresses.emplace_back(InetSocketAddress(ipAddress, port + j));
        // The sink will always listen to the specified ports
        localAddresses.emplace_back(InetSocketAddress(Ipv4Address::GetAny(), port + j));
    }

    ApplicationContainer currentUeClientApps;
    currentUeClientApps.Add(
        trafficMixerHelper.Install(transportProtocol, addresses, remoteHostContainer.Get(0)));

    // Seed the ARP cache by pinging early in the simulation
    // This is a workaround until a static ARP capability is provided
    PingHelper ping(ipAddress);
    pingApps.Add(ping.Install(remoteHostContainer));

    Ptr<NetDevice> ueDevice = ueNetDev.Get(i);
    // Activate a dedicated bearer for the traffic type per node
    nrHelper->ActivateDedicatedEpsBearer(ueDevice, bearer, tft);
    // Activate a dedicated bearer for the traffic type per node
    if (isMx1)
    {
        nrHelper->ActivateDedicatedEpsBearer(ueDevice, bearer, tft);
    }
    else
    {
        NS_ASSERT(tfts.size() >= currentUeClientApps.GetN());
        for (uint32_t j = 0; j < currentUeClientApps.GetN(); j++)
        {
            nrHelper->ActivateDedicatedEpsBearer(ueDevice, bearer, tfts[j]);
        }
    }

    for (uint32_t j = 0; j < currentUeClientApps.GetN(); j++)
    {
        PacketSinkHelper dlPacketSinkHelper(transportProtocol, localAddresses.at(j));
        Ptr<Application> packetSink = dlPacketSinkHelper.Install(ueContainer.Get(i)).Get(0);
        serverApps.Add(packetSink);
    }
    clientApps.Add(currentUeClientApps);
}


int
main(int argc, char* argv[])
{   /*
     GFBR Values added for Modifying QoS-scheduler behavior
    */
    uint64_t gbrDl = 0; // default (kbps)
    uint64_t gbrUl = 0;
    uint64_t mbrUl = 0;
    uint64_t mbrDl = 0;

     // set simulation time and mobility
    uint32_t appDuration = 10000;
    uint32_t appStartTimeMs = 400;

    

    /*
     * Variables that represent the parameters we will accept as input by the
     * command line. Each of them is initialized with a default value, and
     * possibly overridden below when command-line arguments are parsed.
     */
    // Scenario parameters (that we will use inside this script):
    uint16_t gNbNum = 1;
    uint16_t ueNumPergNb = 2;
    bool logging = false;

    // Simulation parameters. Please don't use double to indicate seconds; use
    // ns-3 Time values which use integers to avoid portability issues.
    Time simTime = MilliSeconds(1000);
    Time udpAppStartTime = MilliSeconds(400);

    // NR parameters. We will take the input from the command line, and then we
    // will pass them inside the NR module.
    uint16_t numerology = 0;

    double centralFrequency = 4e9;
    
    


    bool enableOfdma = false;

    uint8_t priorityTrafficScenario = 0; // default is saturation

    uint16_t mcsTable = 2;

    // Where we will store the output files.
    std::string simTag = "flow_results.txt";
    std::string outputDir = "./";

    std::string gbrDlVecStr;
    std::string mbrDlVecStr;
    std::string fiveQiVecStr;
    std::string ueTypeVecStr;
    std::string packetSizeVecStr;
    std::string lambdaVecStr;
    
    uint16_t arUeNum = 0;
    uint16_t vrUeNum = 4;
    uint16_t cgUeNum = 4;
    
    double bandwidth = 10e6;
    double txPower = 41;
    bool isMx1 = true;
    bool useUdp = true;
    double distance = 450;
    uint32_t rngRun = 1;
    double dppV = 0.0;
    bool enableVirtualQueue = true;

    // MAC scheduler type: RR, PF, MR, Qos, DPP
    std::string schedulerType = "DPP";
    bool enableOfdma = true;

    bool logging = false;

    // CommandLine cmd(__FILE__);



    CommandLine cmd;

    cmd.AddValue("gNbNum", "The number of gNbs in multiple-ue topology", gNbNum);
    cmd.AddValue("ueNumPergNb", "The number of UE per gNb in multiple-ue topology", ueNumPergNb);
    cmd.AddValue("logging", "Enable logging", logging);
    cmd.AddValue("priorityTrafficScenario",
                 "The traffic scenario for the case of priority. Can be 0: saturation"
                 "or 1: medium-load",
                 priorityTrafficScenario);
    // cmd.AddValue("simTime", "Simulation time", simTime);
    cmd.AddValue("numerology", "The numerology to be used", numerology);
    cmd.AddValue("centralFrequency", "The system frequency to be used", centralFrequency);
    cmd.AddValue("bandwidth", "The system bandwidth to be used", bandwidth);
    cmd.AddValue("totalTxPower",
                 "total tx power that will be proportionally assigned to"
                 " bands, CCs and bandwidth parts depending on each BWP bandwidth ",
                 txPower);
    cmd.AddValue("simTag",
                 "tag to be appended to output filenames to distinguish simulation campaigns",
                 simTag);
    cmd.AddValue("outputDir", "directory where to store simulation results", outputDir);
    cmd.AddValue("enableOfdma",
                 "If set to true it enables Ofdma scheduler. Default value is false (Tdma)",
                 enableOfdma);

    cmd.AddValue("enableOfdma",
                 "If set to true it enables Ofdma scheduler. Default value is false (Tdma)",
                 enableOfdma),
    cmd.AddValue("schedulerType",
                 "PF: Proportional Fair (default), RR: Round-Robin, Qos",
                 schedulerType);
    cmd.AddValue("dppV", "Drift Plus Penalty V value", dppV);
    cmd.AddValue("enableVirtualQueue", "Enable the throughput virtual queue", enableVirtualQueue);           
 
 
    
    cmd.AddValue("useUdp",
                 "if true, the NGMN applications will run over UDP connection, otherwise a TCP "
                 "connection will be used.",
                 useUdp);
    cmd.AddValue("distance",
                 "The radius of the disc (in meters) that the UEs will be distributed."
                 "Default value is 450m",
                 distance);
    cmd.AddValue("isMx1",
                 "if true M SDFs will be mapped to 1 DRB, otherwise the mapping will "
                 "be 1x1, i.e., 1 SDF to 1 DRB.",
                 isMx1);
    cmd.AddValue("rngRun", "Rng run random number.", rngRun);
    cmd.AddValue("appDuration", "Duration of the application in milliseconds.", appDuration);

    
    // NS_ABORT_MSG_IF(appDuration < 1000, "The appDuration should be at least 1000ms.");
    NS_ABORT_MSG_IF(
        !vrUeNum && !arUeNum && !cgUeNum,
        "Activate at least one type of XR traffic by configuring the number of XR users");

  


        cmd.AddValue("gbrDl", "Guaranteed Bit Rate DL (bps)", gbrDl);
    cmd.AddValue("gbrUl", "Guaranteed Bit Rate UL (bps)", gbrUl);
    cmd.AddValue("mbrDl", "Maximum Bit Rate DL (bps)", mbrDl);
    cmd.AddValue("mbrUl", "Maximum Bit Rate UL (bps)", mbrUl);

    cmd.AddValue("gbrDlVec", "Comma-separated GBR DL per UE", gbrDlVecStr);
    cmd.AddValue("mbrDlVec", "Comma-separated MBR DL per UE", mbrDlVecStr);
    cmd.AddValue("fiveQiVec", "Comma-separated 5QI per UE", fiveQiVecStr);
    cmd.AddValue("ueTypeVec", "Comma-separated UE type (GBR/NGBR) per UE", ueTypeVecStr);
    
    cmd.AddValue("packetSizeVec", "Comma-separated UDP packet size per UE", packetSizeVecStr);
    cmd.AddValue("lambdaVec", "Comma-separated packet rate (pkt/s) per UE", lambdaVecStr);

      cmd.Parse(argc, argv);

       
    // cmd.AddValue("arUeNum", "The number of AR UEs", arUeNum);
    // cmd.AddValue("vrUeNum", "The number of VR UEs", vrUeNum);
    // cmd.AddValue("cgUeNum", "The number of CG UEs", cgUeNum);

    std::vector<uint64_t> gbrDlVec;
    std::stringstream ss(gbrDlVecStr);
    std::vector<uint64_t> mbrDlVec;
    std::stringstream ssmbr(mbrDlVecStr);
    std::string token;
    std::vector<std::string> fiveQiVec;
    std::stringstream ss5qi(fiveQiVecStr);
    std::string token5qi;
    std::vector<std::string> ueTypeVec;
    std::stringstream ssue(ueTypeVecStr);
    std::string tokenUeType;

    std::vector<uint32_t> packetSizeVec;
    std::vector<uint32_t> lambdaVec;
    std::stringstream ssPkt(packetSizeVecStr);
    std::stringstream ssLambda(lambdaVecStr);
    std::string tokenPkt, tokenLambda;

    while (std::getline(ss, token, ','))
    {
        gbrDlVec.push_back(std::stoull(token));
       
    }
     while (std::getline(ssmbr, token, ','))
    {
        mbrDlVec.push_back(std::stoull(token));
       
    }
    while (std::getline(ss5qi, token5qi, ','))
    {
        fiveQiVec.push_back(token5qi);
       
    }
    while (std::getline(ssue, tokenUeType, ','))
    {
        ueTypeVec.push_back(tokenUeType);
    }

    while (std::getline(ssPkt, tokenPkt, ','))
    {
        packetSizeVec.push_back(std::stoul(tokenPkt));
    }

    while (std::getline(ssLambda, tokenLambda, ','))
    {
        lambdaVec.push_back(std::stoul(tokenLambda));
    }


   // Create output directory if it does not exist
    mkdir(outputDir.c_str(), 0777);

    // Change working directory so NR traces go there
    if (chdir(outputDir.c_str()) != 0)
    {
        NS_FATAL_ERROR("Failed to change directory to " << outputDir);
    }

        // enable logging or not
    if (logging)
    {
        //LogLevel logLevel1 =
        //    (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_INFO);
        //LogComponentEnable("NrMacSchedulerNs3", logLevel1);
        //LogComponentEnable("NrMacSchedulerTdma", logLevel1);
        LogComponentEnable("NrMacSchedulerNs3", LOG_LEVEL_WARN);
        // LogComponentEnable("NrMacSchedulerOfdmaDPP", LOG_LEVEL_DEBUG);
    }

    uint32_t simTimeMs = appStartTimeMs + appDuration + 2000;

    // Set simulation run number
    SeedManager::SetRun(rngRun);

    
    // setup the nr simulation
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    // simple band configuration and initialize
    CcBwpCreator ccBwpCreator;
    CcBwpCreator::SimpleOperationBandConf bandConf(centralFrequency,
                                                   bandwidth,
                                                   1,
                                                   BandwidthPartInfo::UMa_LoS);

    OperationBandInfo band = ccBwpCreator.CreateOperationBandContiguousCc(bandConf);
    nrHelper->InitializeOperationBand(&band);
    BandwidthPartInfoPtrVector allBwps = CcBwpCreator::GetAllBwps({band});

    nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(txPower));
    nrHelper->SetGnbPhyAttribute("Numerology", UintegerValue(numerology));
    nrHelper->SetGnbPhyAttribute("NoiseFigure", DoubleValue(5));
    nrHelper->SetUePhyAttribute("TxPower", DoubleValue(23));
    nrHelper->SetUePhyAttribute("NoiseFigure", DoubleValue(7));

    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));
    Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping",
                       EnumValue(useUdp ? LteEnbRrc::RLC_UM_ALWAYS : LteEnbRrc::RLC_AM_ALWAYS));
            
    Config::SetDefault("ns3::NrMacSchedulerOfdmaDPP::DppV", DoubleValue(dppV));
    Config::SetDefault("ns3::NrMacSchedulerOfdmaDPP::enableVirtualQueue", BooleanValue(true));                       

    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                     PointerValue(CreateObject<ThreeGppAntennaModel>()));
    nrHelper->SetGnbAntennaAttribute("AntennaHorizontalSpacing", DoubleValue(0.5));
    nrHelper->SetGnbAntennaAttribute("AntennaVerticalSpacing", DoubleValue(0.8));
    nrHelper->SetGnbAntennaAttribute("DowntiltAngle", DoubleValue(0 * M_PI / 180.0));
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(1));
    nrHelper->SetUeAntennaAttribute("AntennaElement",
                                    PointerValue(CreateObject<IsotropicAntennaModel>()));

    // Set up MAC scheduler
    std::stringstream scheduler;
    std::string subType;
    subType = enableOfdma == false ? "Tdma" : "Ofdma";
    scheduler << "ns3::NrMacScheduler" << subType << schedulerType;
    std::cout << "Scheduler: " << scheduler.str() << std::endl;
    nrHelper->SetSchedulerTypeId(TypeId::LookupByName(scheduler.str()));

    
    // Beamforming method
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(DirectPathBeamforming::GetTypeId()));
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);

    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    nrHelper->SetEpcHelper(epcHelper);
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));

    // Initialize nrHelper
    nrHelper->Initialize();

    NodeContainer gNbNodes;
    NodeContainer ueNodes;
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    const double gNbHeight = 25;
    const double ueHeight = 1.5;

    gNbNodes.Create(1);
    ueNodes.Create(arUeNum + vrUeNum + cgUeNum);

    Ptr<ListPositionAllocator> bsPositionAlloc = CreateObject<ListPositionAllocator>();
    bsPositionAlloc->Add(Vector(0.0, 0.0, gNbHeight));
    mobility.SetPositionAllocator(bsPositionAlloc);
    mobility.Install(gNbNodes);

    Ptr<RandomDiscPositionAllocator> ueDiscPositionAlloc =
        CreateObject<RandomDiscPositionAllocator>();
    ueDiscPositionAlloc->SetX(0.0);
    ueDiscPositionAlloc->SetY(0.0);
    ueDiscPositionAlloc->SetZ(ueHeight);
    mobility.SetPositionAllocator(ueDiscPositionAlloc);

    for (uint32_t i = 0; i < ueNodes.GetN(); i++)
    {
        mobility.Install(ueNodes.Get(i));
    }
