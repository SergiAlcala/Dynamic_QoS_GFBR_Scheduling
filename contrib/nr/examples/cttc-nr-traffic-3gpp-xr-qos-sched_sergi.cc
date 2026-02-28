// Copyright (c) 2023 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only

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
#include "ns3/lte-ue-rrc.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-module.h"
#include "ns3/packet-sink.h"
#include "ns3/point-to-point-module.h"
#include "ns3/xr-traffic-mixer-helper.h"

#include <vector>
#include <fstream>
#include <iomanip>

/**
 * \file cttc-nr-traffic-3gpp-xr_neco.cc
 * \ingroup examples
 * \brief Simple topology consisting of 1 GNB and various UEs.
 *  Can be configured with different 3GPP XR traffic generators (by using
 *  XR traffic mixer helper).
 *
 * To run the simulation with the default configuration one shall run the
 * following in the command line:
 *
 * ./ns3 run cttc-nr-traffic-generator-3gpp-xr_neco
 *
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CttcNrTraffic3gppXrNeco");

std::vector<std::string> ParseStringList(const std::string& input)
{
    std::vector<std::string> result;
    std::stringstream ss(input);
    std::string token;

    while (std::getline(ss, token, '|'))
    {
        if (!token.empty())
            result.push_back(token);
    }
    return result;
}

struct UeDynamicContext
{
    Ptr<TrafficGenerator3gppGenericVideo> video;
    Ptr<NrUeNetDevice> ueDev;
    std::vector<uint64_t> traffic;
    std::vector<uint64_t> gbr;
    uint32_t index;
};



EpsBearer::Qci
GetQciFromString(const std::string& qciStr)
{
    if (qciStr == "GBR_GAMING")
        return EpsBearer::GBR_GAMING;
    if (qciStr == "NGBR_LOW_LAT_EMBB")
        return EpsBearer::NGBR_LOW_LAT_EMBB;
    if (qciStr == "GBR_CONV_VIDEO")
        return EpsBearer::GBR_CONV_VIDEO;

    NS_ABORT_MSG("Unsupported 5QI profile");
}

 std::vector<uint64_t>
    ParseProfile(const std::string& profileStr)
    {
        std::vector<uint64_t> values;
        std::stringstream ss(profileStr);
        std::string token;

        while (std::getline(ss, token, ','))
        {
            double bps = std::stod(token);
            values.push_back(static_cast<uint64_t>(bps));
        }

        return values;
    }

void
PeriodicUpdate(Ptr<TrafficGenerator3gppGenericVideo> video,
               Ptr<NrMacSchedulerOfdmaDPP> sched,
               Ptr<NrUeNetDevice> ueDev,
               std::vector<uint64_t> traffic,
               std::vector<uint64_t> gbr,
               uint32_t index,
               double interval)
{
uint16_t rnti = ueDev->GetRrc()->GetRnti();
    std::cout << "[PERIODIC] Time "
          << Simulator::Now().GetSeconds()
          << " UE " << rnti
          << " index=" << index
          << std::endl;

    static std::ofstream debugFile("debug_updates.csv", std::ios::out);
    static bool headerWritten = false;    

    if (!headerWritten)
    {
        debugFile << "Time(s)\tRNTI\tTraffic(Mbps)\tGFBR(Mbps)\n";
        headerWritten = true;
    }

    if (index >= traffic.size() || index >= gbr.size())
        return;

    uint64_t newTraffic = traffic[index];
    uint64_t newGbr = gbr[index];
    
    // video->SetAttribute("DataRate", DoubleValue(newTraffic / 1e6));
    video->SetDynamicDataRate(newTraffic / 1e6);
    sched->UpdateUeDlGfbr(rnti, newGbr);

    debugFile << Simulator::Now().GetSeconds() << "\t"
              << rnti << "\t"
              << newTraffic / 1e6 << "\t"
              << newGbr / 1e6 << "\n";


    debugFile.flush();   // ensures writing immediately


    std::cout << "Time "
              << Simulator::Now().GetSeconds()
              << "s → Traffic="
              << newTraffic/1e6
              << " Mbps, GFBR="
              << newGbr/1e6
              << " Mbps"
              << std::endl;

    Simulator::Schedule(Seconds(interval),
    [=]() {
        PeriodicUpdate(video,
                       sched,
                       ueDev,
                       traffic,
                       gbr,
                       index + 1,
                       interval);
    });

    
}

void
CalculateThroughputAll(std::vector<Ptr<PacketSink>> sinks,
                       std::vector<uint64_t> *lastTotalRx,
                       std::ofstream *outFile,
                       double interval)
{
    double now = Simulator::Now().GetSeconds();

    (*outFile) << now;

    for (uint32_t i = 0; i < sinks.size(); ++i)
    {
        uint64_t cur = sinks[i]->GetTotalRx();
        double throughput = (cur - (*lastTotalRx)[i]) * 8.0
                            / (interval * 1e6);  // Mbps

        (*outFile) << "," << throughput;

        (*lastTotalRx)[i] = cur;
    }

    (*outFile) << std::endl;

    Simulator::Schedule(Seconds(interval),
                        &CalculateThroughputAll,
                        sinks,
                        lastTotalRx,
                        outFile,
                        interval);
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

std::vector<Ptr<TrafficGenerator3gppGenericVideo>>
ConfigureXrApp(NodeContainer& ueContainer,
               Ipv4InterfaceContainer& ueIpIface,
               enum NrXrConfig config,
               uint16_t portStart,
               std::string transportProtocol,
               NodeContainer& remoteHostContainer,
               NetDeviceContainer& ueNetDev,
               Ptr<NrHelper> nrHelper,
               const std::vector<std::string>& fiveQiStrings,
               ApplicationContainer& serverApps,
               ApplicationContainer& clientApps,
               ApplicationContainer& pingApps,
               double initialDatarate)
{
   std::vector<Ptr<TrafficGenerator3gppGenericVideo>> videoPtrs;

for (uint32_t i = 0; i < ueContainer.GetN(); ++i)
{
    Ipv4Address ipAddress = ueIpIface.GetAddress(i, 0);

    uint16_t port = portStart + i;

    std::vector<Address> addresses;
    std::vector<InetSocketAddress> localAddresses;

    addresses.emplace_back(InetSocketAddress(ipAddress, port));
    localAddresses.emplace_back(InetSocketAddress(Ipv4Address::GetAny(), port));

    XrTrafficMixerHelper trafficMixerHelper;
    trafficMixerHelper.ConfigureXr(config);

    ApplicationContainer currentUeClientApps =
        trafficMixerHelper.Install(
            transportProtocol,
            addresses,
            remoteHostContainer.Get(0));

    // ---- SET APPLICATION ATTRIBUTES ----
    for (uint32_t j = 0; j < currentUeClientApps.GetN(); j++)
    {
        Ptr<TrafficGenerator3gppGenericVideo> video =
            DynamicCast<TrafficGenerator3gppGenericVideo>(
                currentUeClientApps.Get(j));

        if (video)
      
            {
                video->SetAttribute("Fps", UintegerValue(60));

                videoPtrs.push_back(video);

                std::cout << "[APP CREATED] UE index=" << i
                          << " ptr=" << video.operator->()
                          << std::endl;
            }
    }

    // ---- SEED ARP CACHE (RESTORED) ----
    PingHelper ping(ipAddress);
    pingApps.Add(ping.Install(remoteHostContainer));

    // ---- PER-UE 5QI ----
    EpsBearer::Qci qci =
        GetQciFromString(fiveQiStrings[i]);

    GbrQosInformation qos;
    qos.gbrDl = 0;  // dynamic later

    EpsBearer bearer(qci, qos);

    Ptr<EpcTft> tft = Create<EpcTft>();
    EpcTft::PacketFilter dlpf;
    dlpf.localPortStart = port;
    dlpf.localPortEnd = port;
    tft->Add(dlpf);

    Ptr<NetDevice> ueDevice = ueNetDev.Get(i);

    nrHelper->ActivateDedicatedEpsBearer(
        ueDevice, bearer, tft);

    // ---- PACKET SINK INSTALLATION (RESTORED) ----
    for (uint32_t j = 0; j < currentUeClientApps.GetN(); j++)
    {
        PacketSinkHelper dlPacketSinkHelper(
            transportProtocol,
            localAddresses.at(j));

        Ptr<Application> packetSink =
            dlPacketSinkHelper.Install(
                ueContainer.Get(i)).Get(0);

        serverApps.Add(packetSink);
    }

    clientApps.Add(currentUeClientApps);
}

return videoPtrs;
}

int
main(int argc, char* argv[])
{
    // set simulation time and mobility
    uint32_t appDuration = 10000;
    uint32_t appStartTimeMs = 400;
    uint16_t numerology = 1; // Default is 0, but we need more bandwidth to accommodate the traffic profiles with the default parameters 
    
    uint16_t vrUeNum = 4;
    
    double centralFrequency = 4e9;
    double bandwidth = 100e6; // Default is 10Mhz, but we need more bandwidth to accommodate the traffic profiles with the default parameters
    double txPower = 46 ;  // 41 original
    bool isMx1 = true;
    bool useUdp = true;
    double distance = 50; // 450 original
    uint32_t rngRun = 1;
    double dppV = 0.0;
    bool enableVirtualQueue = true;

    double Datarate = 5; // Mbps, default value for the video traffic generator

    uint64_t CGgbrDL = 0; // default (bps)
    std::string rateProfileStr;

    std::string trafficProfileStr;
    std::string gbrProfileStr;
    double traceInterval = 1.0;
    std::vector<Ptr<TrafficGenerator3gppGenericVideo>> allVideoPtrs;

    
    

    std::vector<UeDynamicContext> dynamicUes;
    std::string fiveQiProfilesStr;

    double trafficInterval = 0.2;   // fast scale
    double gfbrInterval = 30.0;     // slow scale



    // MAC scheduler type: RR, PF, MR, Qos, DPP
    std::string schedulerType = "DPP";
    bool enableOfdma = true;

    bool logging = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("enableOfdma",
                 "If set to true it enables Ofdma scheduler. Default value is false (Tdma)",
                 enableOfdma),
    cmd.AddValue("schedulerType",
                 "PF: Proportional Fair (default), RR: Round-Robin, Qos",
                 schedulerType);
    cmd.AddValue("dppV", "Drift Plus Penalty V value", dppV);
    cmd.AddValue("enableVirtualQueue", "Enable the throughput virtual queue", enableVirtualQueue);           
    cmd.AddValue("logging", "Enable logging", logging);
    
    cmd.AddValue("vrUeNum", "The number of VR UEs", vrUeNum);
    
    cmd.AddValue("numerology", "The numerology to be used.", numerology);
    cmd.AddValue("txPower", "Tx power to be configured to gNB", txPower);
    cmd.AddValue("frequency", "The system frequency", centralFrequency);
    cmd.AddValue("bandwidth", "The system bandwidth", bandwidth);
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
    
    
    cmd.AddValue("trafficProfile", "Comma-separated traffic profile (bps)",trafficProfileStr);
    cmd.AddValue("gbrProfile", "Comma-separated GFBR profile (bps)",gbrProfileStr);
    cmd.AddValue("traceInterval","Update interval (seconds)",traceInterval);
    cmd.AddValue("fiveQiProfiles", "Per UE 5QI separated by |",fiveQiProfilesStr);
    cmd.AddValue("trafficInterval", "Traffic update interval (seconds)", trafficInterval);
    cmd.AddValue("gfbrInterval", "GFBR update interval (seconds)", gfbrInterval);
    cmd.Parse(argc, argv);

    // NS_ABORT_MSG_IF(appDuration < 1000, "The appDuration should be at least 1000ms.");
    NS_ABORT_MSG_IF(
        !vrUeNum ,
        "Activate at least one type of XR traffic by configuring the number of XR users");

    std::vector<double> dynamicRates;
    std::stringstream ss(rateProfileStr);
    std::string token;

    while (std::getline(ss, token, ','))
    {
        dynamicRates.push_back(std::stod(token));
    }

    
    
    std::vector<uint64_t> trafficProfile;
    std::vector<uint64_t> gbrProfile;

    auto fiveQiStrings = ParseStringList(fiveQiProfilesStr);
    auto trafficStrings = ParseStringList(trafficProfileStr);
    auto gbrStrings = ParseStringList(gbrProfileStr);


   
   

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
    ueNodes.Create( vrUeNum );

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

    /*
     * Create various NodeContainer(s) for the different traffic types.
     * In ueArContainer, ueVrContainer, ueCgContainer, we will put
     * AR, VR, CG UEs, respectively.*/
    
    NodeContainer ueVrContainer;
    

    for (auto j = 0; j < vrUeNum; ++j)
    {
        Ptr<Node> ue = ueNodes.Get(j);
        ueVrContainer.Add(ue);
    }



    NetDeviceContainer gNbNetDev = nrHelper->InstallGnbDevice(gNbNodes, allBwps);
    
    NetDeviceContainer ueVrNetDev = nrHelper->InstallUeDevice(ueVrContainer, allBwps);
    
    //////////////////////////////////////////////////
    Ptr<NrMacScheduler> baseSched =
    nrHelper->GetScheduler(gNbNetDev.Get(0), 0); // get the scheduler of the first gNB and first BWP

    Ptr<NrMacSchedulerOfdmaDPP> dppSched =
    DynamicCast<NrMacSchedulerOfdmaDPP>(baseSched);

    NS_ASSERT(dppSched);

    //////////////////////////////////////////////////

    int64_t randomStream = 1;
    randomStream += nrHelper->AssignStreams(gNbNetDev, randomStream);
    
    randomStream += nrHelper->AssignStreams(ueVrNetDev, randomStream);
    

    for (auto it = gNbNetDev.Begin(); it != gNbNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }
  
    for (auto it = ueVrNetDev.Begin(); it != ueVrNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }
 

    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);

    // connect a remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1000));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.000)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);

    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    internet.Install(ueNodes);

    
    Ipv4InterfaceContainer ueVrIpIface;
    

    
    ueVrIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueVrNetDev));
    

    // Set the default gateway for the UEs
    for (uint32_t j = 0; j < ueNodes.GetN(); ++j)
    {
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ueNodes.Get(j)->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
    }

    // attach UEs to the closest eNB
    
    nrHelper->AttachToClosestEnb(ueVrNetDev, gNbNetDev);
    



    // Install sink application
    ApplicationContainer serverApps;

    // configure the transport protocol to be used
    std::string transportProtocol;
    transportProtocol = useUdp == true ? "ns3::UdpSocketFactory" : "ns3::TcpSocketFactory";
    uint16_t dlPortVrStart = 1131;
      



    Ptr<EpcTft> vrTft = Create<EpcTft>();
    EpcTft::PacketFilter dlpfVr;
    dlpfVr.localPortStart = dlPortVrStart;
    dlpfVr.localPortEnd = dlPortVrStart;
    vrTft->Add(dlpfVr);

 
 

    // Install traffic generators
    ApplicationContainer clientApps;
    ApplicationContainer pingApps;

    std::ostringstream xrFileTag;


    auto videoPtrs = ConfigureXrApp(
    ueVrContainer,
    ueVrIpIface,
    VR_DL1,
    dlPortVrStart,
    transportProtocol,
    remoteHostContainer,
    ueVrNetDev,
    nrHelper,
    fiveQiStrings,
    serverApps,
    clientApps,
    pingApps,
    Datarate);

    allVideoPtrs = videoPtrs;
    NS_ASSERT(allVideoPtrs.size() == trafficStrings.size());

    for (uint32_t i = 0; i < allVideoPtrs.size(); ++i)
    {
        UeDynamicContext ctx;
        ctx.video = allVideoPtrs[i];
        // ctx.rnti = rntiList[i];
        ctx.ueDev = DynamicCast<NrUeNetDevice>(ueVrNetDev.Get(i));

        // ctx.traffic = trafficProfile;   // full vector
        // ctx.gbr = gbrProfile;      
        ctx.traffic = ParseProfile(trafficStrings[i]);
        ctx.gbr = ParseProfile(gbrStrings[i]);     // full vector
        ctx.index = 0;   // IMPORTANT

        dynamicUes.push_back(ctx);

        std::cout << "[CTX CREATED] UE " << i
              << " traffic[0]=" << ctx.traffic[0]
              << " ptr=" << ctx.video.operator->()
              << std::endl;
    }

    std::cout << "videoPtrs size = " << videoPtrs.size() << std::endl;
    std::cout << "allVideoPtrs size = " << allVideoPtrs.size() << std::endl;
    std::cout << "trafficProfile size = " << trafficStrings.size() << std::endl;
    std::cout << "gbrProfile size = " << gbrStrings.size() << std::endl;

    
    

    pingApps.Start(MilliSeconds(100));
    pingApps.Stop(MilliSeconds(appStartTimeMs));

    // start server and client apps
    serverApps.Start(MilliSeconds(appStartTimeMs));
    clientApps.Start(MilliSeconds(appStartTimeMs));
    serverApps.Stop(MilliSeconds(simTimeMs));
    clientApps.Stop(MilliSeconds(appStartTimeMs + appDuration));



    std::vector<Ptr<PacketSink>> sinks;



for (uint32_t i = 0; i < serverApps.GetN(); ++i)
{
    Ptr<PacketSink> sink =
        DynamicCast<PacketSink>(serverApps.Get(i));

    if (sink)
        sinks.push_back(sink);
}
std::vector<uint64_t> *lastTotalRx =
    new std::vector<uint64_t>(sinks.size(), 0);

std::ofstream thputFile("throughput_per_ue.csv");

thputFile << "Time";
for (uint32_t i = 0; i < sinks.size(); ++i)
{
    thputFile << ",UE" << i;
}
thputFile << std::endl;

double interval = traceInterval;
// double interval = 0.2;  // 200 ms

Simulator::Schedule(Seconds(interval),
                    &CalculateThroughputAll,
                    sinks,
                    lastTotalRx,
                    &thputFile,
                    interval);

for (auto &ctx : dynamicUes)
{
    double initialRateBps = ctx.traffic[0];


    ctx.video->SetAttribute(
        "DataRate",
        DoubleValue(initialRateBps / 1e6)  // Mbps
    );

    std::cout << "[INITIAL SET] ptr=" << ctx.video.operator->()
              << " rate=" << initialRateBps / 1e6
              << " Mbps"
              << std::endl;
   

    Simulator::Schedule(Seconds(appStartTimeMs/1000.0),
    [=]() {
        PeriodicUpdate(ctx.video,
               dppSched,
               ctx.ueDev,
               ctx.traffic,
               ctx.gbr,
               0u,
               interval);
    });
                    
}
std::cout << "dynamicUes size = " << dynamicUes.size() << std::endl;





    FlowMonitorHelper flowmonHelper;
    NodeContainer endpointNodes;
    endpointNodes.Add(remoteHost);
    endpointNodes.Add(ueNodes);

    Ptr<ns3::FlowMonitor> monitor = flowmonHelper.Install(endpointNodes);
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.0001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(20));

    Simulator::Stop(MilliSeconds(simTimeMs));
    Simulator::Run();


    /////// This part is for writing the results in a file dynamically, for tmux ////////
    // std::ostringstream filename;

    // // Convert CGgbrDL to Mbps (like in your python int(CGgbrDL/1e6))
    // uint64_t cgGbrMbps = CGgbrDL / 1000000;

    // // Build filename exactly like your python script
    // filename << "sim_DR_GFBR_analysis/"
    //         << cgUeNum << "_CG_" << vrUeNum << "_VR/"
    //         << "res" << schedulerType << "_"
    //         << Datarate << "_DR_"
    //         << cgGbrMbps << "_GFBR_"
    //         << cgUeNum << "_CG_"
    //         << vrUeNum << "_VR.txt";

    // // Create directory if needed (Linux/macOS)
    // std::string dir = "sim_DR_GFBR_analysis/" +
    //                 std::to_string(cgUeNum) + "_CG_" +
    //                 std::to_string(vrUeNum) + "_VR/";

    // system(("mkdir -p " + dir).c_str());

    // std::ofstream outputFile(filename.str());

    /////////////////////////////////////////////////////////////////////////////////////////
    
    std::ofstream outputFile("res.txt");

    
    

    outputFile << "Thput\tRx\tDuration\n";

    // Print per-flow statistics
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    double averageFlowThroughput = 0.0;
    double averageFlowDelay = 0.0;

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::stringstream protoStream;
        protoStream << (uint16_t)t.protocol;
        if (t.protocol == 6)
        {
            protoStream.str("TCP");
        }
        if (t.protocol == 17)
        {
            protoStream.str("UDP");
        }

        Time txDuration = MilliSeconds(appDuration);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << ":" << t.sourcePort << " -> "
                  << t.destinationAddress << ":" << t.destinationPort << ") proto "
                  << protoStream.str() << "\n";
        std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
        std::cout << "  TxOffered:  "
                  << ((i->second.txBytes * 8.0) / txDuration.GetSeconds()) * 1e-6 << " Mbps\n";
        std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";

        if (i->second.rxPackets > 0)
        {
            // Measure the duration of the flow from receiver's perspective
            Time rxDuration = i->second.timeLastRxPacket - i->second.timeFirstTxPacket;
            averageFlowThroughput += ((i->second.rxBytes * 8.0) / rxDuration.GetSeconds()) * 1e-6;
            averageFlowDelay += 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets;

            double throughput = ((i->second.rxBytes * 8.0) / rxDuration.GetSeconds()) * 1e-6;
            double delay = 1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets;
            double jitter = 1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets;

            std::cout << "  Throughput: " << throughput << " Mbps\n";
            // std::cout << i->second.rxBytes * 8.0 << " / " << rxDuration.GetSeconds() << std::endl;
            // std::cout << "tf = " << i->second.timeLastRxPacket << " t0 = " << i->second.timeFirstTxPacket << std::endl;
            // double thput_prueba = ((i->second.rxBytes * 8.0) / txDuration.GetSeconds()) * 1e-6;
            // std::cout << "  Throughput*: " << thput_prueba << " Mbps\n";
            std::cout << "  Mean delay:  " << delay << " ms\n";
            std::cout << "  Mean jitter:  " << jitter << " ms\n";

            outputFile << throughput << "\t" << i->second.rxBytes * 8.0 << "\t" << rxDuration.GetSeconds() << std::endl;
        }
        else
        {
            std::cout << "  Throughput:  0 Mbps\n";
            std::cout << "  Mean delay:  0 ms\n";
            std::cout << "  Mean upt:  0  Mbps \n";
            std::cout << "  Mean jitter: 0 ms\n";

            outputFile << 0 << "\t" << 0 << "\t" << 0 << std::endl;
        }
        std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
    }

    std::cout << "\n\n  Mean flow throughput: " << averageFlowThroughput / stats.size()
              << "Mbps \n";
    std::cout << "  Mean flow delay: " << averageFlowDelay / stats.size() << " ms\n";



    Simulator::Destroy();

    return 0;
}
