
import numpy as np
import subprocess
import random
app_duration = 450000

file="scenario_8_ue_4k.json"
# file = 'scenario_8_ue_480p.json'
# file = "scenario_1_ue_4k.json"
# file = 'scenario_4_ue_480p_4ue_NGBR.json'
# file = 'scenario_20_ue_4K_15_GBR_5_NGBR.json'
# file = 'scenario_20_ue_4K.json'
# file = 'scenario_4_ue_4K_4ue_NGBR.json'
CONFIG_FILE = f"/home/sergi/5glena-lyapunov-mac-scheduler/predictions/{file}"

# CONFIG_FILE = "/home/sergi/5glena-lyapunov-mac-scheduler/scenario.json"
dppV = 0
# ===============================
# NS-3 LAUNCHER
# ===============================

dppV = 0
bandwidth = 100e6
numerology = 1
txPower = 46
distance = 50
schedulerType = "DPP"
dppV = 0.0
enableOfdma = 'true'
configFile = CONFIG_FILE
app_duration = app_duration
unified_name = f"unified_stats_{file.replace('.json', '')}_{schedulerType}_new_variables.csv"
symbolsPerSec = 14000.0 if schedulerType == "QoS" and numerology == 0 else 28000.0 if schedulerType == "QoS" and numerology == 1 else None
timeslot = 1e-3 if schedulerType == "DPP" and numerology == 0 else 0.5e-3 if schedulerType == "DPP" and numerology == 1 else None


# cmd = [
#     "./ns3", "run",
#     f"cttc-nr-traffic-3gpp-xr-qos-sched_sergi  --enableOfdma=true --configFile={CONFIG_FILE} --appDuration={app_duration} --dppV={dppV}"
# ]
cmd = [
    "./ns3", "run",
    f"cttc-nr-traffic-3gpp-xr-qos-sched_sergi  --enableOfdma={enableOfdma} --configFile={CONFIG_FILE} --appDuration={app_duration}\
        --bandwidth={bandwidth} \
        --numerology={numerology} \
        --txPower={txPower} \
        --distance={distance} \
        --schedulerType={schedulerType} \
        --unifiedName={unified_name} \
        --dppTimeSlot={timeslot}"
]
# cmd = [
#     "./ns3", "run",
#     f"cttc-nr-traffic-3gpp-xr-qos-sched_sergi  --enableOfdma=true --configFile={CONFIG_FILE} --appDuration={app_duration} --dppV={dppV}"
# ]
# cmd = [
#     "./ns3", "run",
#     f"cttc-nr-traffic-3gpp-xr-qos-sched_sergi --trafficProfile={traffic_str} --gbrProfile={gbr_str} --fiveQiProfiles={fiveqi_str} --traceInterval={TRACE_INTERVAL} --schedulerType={SCHEDULER} --enableOfdma=true --vrUeNum={NUM_UES} --appDuration={app_duration}"
# ]


subprocess.run(cmd)
# subprocess.run(cmd,stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
