import csv
import numpy as np
from matplotlib import pyplot as plt
import os
import subprocess
from multiprocessing import Pool


vrues=[0,1,2,3,4]
cgues=[1,2,3,4]

CGgbrDLs=[ 1e6, 5e6, 10e6, 15e6, 20e6, 25e6, 30e6, 35e6, 40e6]
# CGgbrDL= 15e6
Datarates=[1, 5, 10, 15, 20, 25, 30, 35, 40]
# Datarate=5
# Run schedulers
num_sim = 1 # Number of simulations

def myMultiOpt(idx):
    Datarate, CGgbrDL, cg, vr = idx
    print(f'Pair: {idx}')
    filepath = f'{cg}_CG_{vr}_VR'
    if not os.path.exists(f"sim_DR_GFBR_analysis/{filepath}/"):
        os.makedirs(f"sim_DR_GFBR_analysis/{filepath}")



    cmd = ["./ns3", "run",
            f"cttc-nr-traffic-3gpp-xr-qos-sched_sergi --Datarate={Datarate} --appDuration=10000 --arUeNum=0 --vrUeNum={vr} --cgUeNum={cg} --enableOfdma=true --schedulerType=DPP --CGgbrDL={CGgbrDL} > /dev/null 2>&1"
            ]
    # os.system(cmd)
    subprocess.run(cmd)
    # cmd = f"mv res{CGgbrDL}.txt sim_DR_GFBR_analysis/{filepath}/resDPP_{Datarate}_DR_{int(CGgbrDL/1e6)}_GFBR_{cg}_CG_{vr}_VR.txt"
    # os.system(cmd)
    cmd = ["./ns3", "run",
            f"cttc-nr-traffic-3gpp-xr-qos-sched_sergi --Datarate={Datarate} --appDuration=10000 --arUeNum=0 --vrUeNum={vr} --cgUeNum={cg} --enableOfdma=true --schedulerType=Qos --CGgbrDL={CGgbrDL} > /dev/null 2>&1"
            ]
    # os.system(cmd)
    subprocess.run(cmd)
    # cmd = f"mv res{CGgbrDL}.txt sim_DR_GFBR_analysis/{filepath}/resQos_{Datarate}_DR_{int(CGgbrDL/1e6)}_GFBR_{cg}_CG_{vr}_VR.txt"
    # os.system(cmd)
    #antennas_distance=num_BS

pair_list=[]

for drs in range(len(Datarates)):
    for cgbrs in range(len(CGgbrDLs)):
         for cg in cgues:
              for vr in vrues:
                   pair_list.append((Datarates[drs], CGgbrDLs[cgbrs], cg, vr))
            
        
if __name__ == '__main__':
    with Pool(100) as p:
        p.map(myMultiOpt,pair_list)
