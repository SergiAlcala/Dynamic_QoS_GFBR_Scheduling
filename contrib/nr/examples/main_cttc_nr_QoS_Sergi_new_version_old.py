import subprocess
import os, sys


# Define UE profiles with all necessary parameters for both GBR and NGBR types

# ue_profiles = [
#     {"type": "GBR",  "5qi": "GBR_CONV_VIDEO","gbrDl": 12e6, "mbrDl": 25e6, "packetSize": 3000, "lambda": 1000},
#     # {"type": "GBR",  "5qi": "GBR_CONV_VIDEO","gbrDl": 12e6, "mbrDl": 6e6, "packetSize": 1500, "lambda": 1000},
#     # {"type": "NGBR", "5qi": "NGBR_LOW_LAT_EMBB", "packetSize": 1500, "lambda": 1000},
#     {"type": "NGBR", "5qi": "NGBR_LOW_LAT_EMBB","packetSize": 3000, "lambda": 1000},
# ]
ue_profiles = [
    {"type": "GBR",  "5qi": "GBR_CONV_VOICE","gbrDl": 5e6, "mbrDl": 30e6, "packetSize": 3000, "lambda": 1000}, 
    {"type": "GBR",  "5qi": "GBR_CONV_VOICE","gbrDl": 5e6, "mbrDl": 30e6, "packetSize": 3000, "lambda": 1000}, 
    {"type": "GBR",  "5qi": "GBR_CONV_VOICE","gbrDl": 5e6, "mbrDl": 30e6, "packetSize": 3000, "lambda": 1000}, 
    # {"type": "GBR",  "5qi": "GBR_CONV_VOICE","gbrDl": 5e6, "mbrDl": 30e6, "packetSize": 3000, "lambda": 1000}, 
    # {"type": "GBR",  "5qi": "GBR_CONV_VIDEO","gbrDl": 10e6, "mbrDl": 20e6, "packetSize": 3000, "lambda": 1000}, 
    # {"type": "GBR",  "5qi": "GBR_CONV_VIDEO","gbrDl": 10e6, "mbrDl": 20e6, "packetSize": 3000, "lambda": 1000}, 
    # {"type": "GBR",  "5qi": "GBR_CONV_VIDEO","gbrDl": 10e6, "mbrDl": 20e6, "packetSize": 3000, "lambda": 1000}, 
    # {"type": "GBR",  "5qi": "GBR_CONV_VIDEO","gbrDl": 10e6, "mbrDl": 20e6, "packetSize": 3000, "lambda": 1000}, 
    # {"type": "GBR",  "5qi": "GBR_CONV_VIDEO","gbrDl": 10e6, "mbrDl": 20e6, "packetSize": 3000, "lambda": 1000},     
    # # {"type": "GBR",  "5qi": "GBR_CONV_VIDEO","gbrDl": 30e6, "mbrDl": 20e6, "packetSize": 3000, "lambda": 1000},
    # {"type": "NGBR", "5qi": "NGBR_LOW_LAT_EMBB","packetSize": 3000, "lambda": 1000},
# // std::ofstream outputFile("res" + std::to_string(CGgbrDL) + ".txt");
]


#  UE number per gNb is determined by the length of the ue_profiles list
ueNumPergNb = len(ue_profiles)

#  Sanity check: Ensure GBR profiles have the necessary parameters
for i, p in enumerate(ue_profiles):
    if p["type"] == "GBR":
        assert "gbrDl" in p and "mbrDl" in p, f"Missing GBR params for UE {i}"


# Convert UE profiles into comma-separated strings for command-line arguments
ueTypeVec = ",".join(p["type"] for p in ue_profiles)
fiveQiVec = ",".join(p["5qi"] for p in ue_profiles)
gbrDlVec = ",".join(str(int(p.get("gbrDl", 0))) for p in ue_profiles)
mbrDlVec = ",".join(str(int(p.get("mbrDl", 0))) for p in ue_profiles)
packetSizeVec = ",".join(str(p["packetSize"]) for p in ue_profiles)
lambdaVec = ",".join(str(p["lambda"]) for p in ue_profiles)

# Extract GBR and NGBR profiles for metadata generation
gbr_profile = next(p for p in ue_profiles if p["type"] == "GBR")
# ngbr_profile = next(p for p in ue_profiles if p["type"] == "NGBR")


# Simulation parameters
simTime= 3 # seconds
gNbNum = 1 # The number of gNbs in multiple-ue topology
numerology = 0
centralFrequency = 4e9  # Hz
bandwidth = 10e6  # Hz
totalTxPower = 40  # dBm before 43 dBm, but to better see the effect of the QoS scheduler, we need to decrease the total transmission power of the gNb. So we set it to 30 dBm, which is a typical value for a small cell deployment.
enableOfdma = True

priorityTrafficScenario = 0  # 0: saturation, 1: medium-load  NOT USED IN THIS EXAMPLE, AS WE CAN CONTROL THE LOAD VIA THE UE PROFILES

# Enable logging for debugging purposes (set to True to enable)
logging = False

# Create output directory structure based on UE profiles
baseOutputDir = "./results/Check_GFBR"
ueDirName = f"{ueNumPergNb}_UE"
ueDir = os.path.join(baseOutputDir, ueDirName)

def profile_tag(p, idx):
    """
    idx is the UE index, so tags remain unambiguous
    """
    if p["type"] == "GBR":
        return (
            f"UE{idx}_"
            f"{p['5qi']}_"
            f"gbrl_{p['gbrDl']/1e6}mbps_"
            f"mbrl_{p['mbrDl']/1e6}mbps_"
            f"pkt_{p['packetSize']}_"
            f"lam_{p['lambda']}"
        )
    else:
        return (
        f"UE{idx}_"
        f"{p['5qi']}_"
        f"pkt_{p['packetSize']}_"
        f"lam_{p['lambda']}"
    )

gbr_tags = []
ngbr_tags = []

for i, p in enumerate(ue_profiles):
    tag = profile_tag(p, i)
    if p["type"] == "GBR":
        gbr_tags.append(tag)
    else:
        ngbr_tags.append(tag)

gbr_part = "GBR_" + "_".join(gbr_tags) if gbr_tags else "GBR_NONE"
ngbr_part = "NGBR_" + "_".join(ngbr_tags) if ngbr_tags else "NGBR_NONE"

# scenarioDirName = f"{gbr_part}__{ngbr_part}"
scenarioDirName = f"test_"
scenarioDir = os.path.join(ueDir, scenarioDirName)

os.makedirs(ueDir, exist_ok=True)
os.makedirs(scenarioDir, exist_ok=True)

# Construct the command to run the ns-3 simulation with the specified parameters
cmd = [
"./ns3", "run",
f"cttc-nr-simple-qos-SERGI "
f"--ueNumPergNb={ueNumPergNb} "
f"--gbrDlVec={gbrDlVec} "
f"--mbrDlVec={mbrDlVec} "
f"--fiveQiVec={fiveQiVec} "
f"--ueTypeVec={ueTypeVec} "    
f"--packetSizeVec={packetSizeVec} "
f"--lambdaVec={lambdaVec} "
f"--simTime={simTime} "
f"--outputDir={scenarioDir} "
f"--gNbNum={gNbNum} "
f"--logging={str(logging).lower()} "
f"--numerology={numerology} "
f"--centralFrequency={centralFrequency} "
f"--bandwidth={bandwidth} "
f"--totalTxPower={totalTxPower} "
f"--enableOfdma={str(enableOfdma).lower()} "
f"--priorityTrafficScenario={priorityTrafficScenario}"
]
       
subprocess.run(cmd)
    


import json
from datetime import datetime

# Generate metadata for the scenario
metadata = {
    "timestamp": datetime.utcnow().isoformat() + "Z",
    "ue_count": ueNumPergNb,
    "gNbNum": gNbNum,
    "radio": {
        "numerology": numerology,
        "bandwidth_hz": bandwidth,
        "central_frequency_hz": centralFrequency,
        "total_tx_power_dbm": totalTxPower,
        "ofdma_enabled": enableOfdma,
    },
    "traffic": [],
    "simulation": {
        "sim_time_s": simTime,
        "priorityTrafficScenario": priorityTrafficScenario,
    }
}
# Add UE profiles to metadata
for i, p in enumerate(ue_profiles):
    offered_mbps = p["packetSize"] * p["lambda"] * 8 / 1e6

    ue_entry = {
        "ue_id": i,
        "type": p["type"],
        "five_qi": p["5qi"],
        "packet_size_bytes": p["packetSize"],
        "lambda_pkt_per_s": p["lambda"],
        "offered_load_mbps": offered_mbps,
    }

    if p["type"] == "GBR":
        ue_entry["gbr_dl_bps"] = p["gbrDl"]
        ue_entry["mbr_dl_bps"] = p["mbrDl"]

    metadata["traffic"].append(ue_entry)

metadata_path = os.path.join(scenarioDir, "metadata.json")

# Save metadata to a JSON file in the scenario directory
with open(metadata_path, "w") as f:
    json.dump(metadata, f, indent=4)