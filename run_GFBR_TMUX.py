import os
import subprocess
import concurrent.futures
import pandas as pd
import numpy as np

# ===============================
# SIMULATION PARAMETERS
# ===============================
APP_DURATION = 450000
BANDWIDTH = 100e6
NUMEROLOGY = 1
TX_POWER = 46
DISTANCE = 50
# SCHEDULER_TYPE = "RR"  # Change to "DPP" for DPP scheduler
ENABLE_OFDMA = 'true'

# Ensure this points to where you saved the 10 JSON files!
CONFIG_DIR = "/home/sergi/5glena-lyapunov-mac-scheduler/predictions/final_scenarios/"

def run_simulation(configuration):
    file_name, scheduler,buffersize, rng_run = configuration
    """This function is executed by a single core to run one simulation."""
    print(f"⏳ [{file_name} _ {scheduler} ] Starting simulation...")
    
    config_file_path = os.path.join(CONFIG_DIR, file_name)

    print (config_file_path)
    filedir = f'final_results/buffer_{buffersize}bps/sim_{rng_run}'
    if not os.path.exists(filedir):
        os.makedirs(filedir)
    unified_name = f"{filedir}/unified_stats_{file_name.replace('.json', '')}_{scheduler}_{BANDWIDTH/1e6}MHz.csv"
    if os.path.exists(unified_name):
    
        # print(f"⚠️  [{file_name} _ {scheduler} ] WARNING: Output file {unified_name} already exists. Skipping simulation.")
        df = pd.read_csv(unified_name)
        last_time = df.iloc[-1,0]
        print(f'last time: {last_time}')

        if last_time >= APP_DURATION/1000:
            
            print(f"✅ [{file_name} _ {scheduler} ] Output file {unified_name} is complete. Skipping simulation.")
            return
        else:
            print(f"⚠️  [{file_name} _ {scheduler} ] Output file {unified_name} is incomplete (last time: {last_time}). Re-running simulation.")
            os.remove(unified_name)  # Remove the incomplete file before re-running
            
    
        
    
    # Calculate dynamic timeslot based on numerology and scheduler
    timeslot = 1e-3 if scheduler == "DPP" and NUMEROLOGY == 0 else 0.5e-3 if scheduler == "DPP" and NUMEROLOGY == 1 else 0

    # Build the target string exactly as ns-3 expects it
    target_string = (
        f"cttc-nr-traffic-3gpp-xr-qos-sched_sergi "
        f"--enableOfdma={ENABLE_OFDMA} "
        f"--configFile={config_file_path} "
        f"--appDuration={APP_DURATION} "
        f"--bandwidth={BANDWIDTH} "
        f"--numerology={NUMEROLOGY} "
        f"--txPower={TX_POWER} "
        f"--distance={DISTANCE} "
        f"--schedulerType={scheduler} "
        f"--unifiedName={unified_name} "
        f"--dppTimeSlot={timeslot} "
        f"--buffersize={buffersize} "  # Set buffer size to 1.25 MB (10 ms at 1 Gbps). Adjust as needed.
        f"--rngRun={rng_run}"  # Add rngRun to ensure different random seeds for each simulation
    )

    cmd = ["./ns3", "run", target_string]

    try:
        # Run the command. stdout=subprocess.DEVNULL hides the massive ns-3 terminal spam 
        # so you can actually read the python print statements!
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
        # subprocess.run(cmd)
        print(f"✅ [{file_name}] SUCCESS! Results saved to {unified_name}")
    except subprocess.CalledProcessError as e:
        print(f"❌ [{file_name}] FAILED! Error code: {e.returncode}")

if __name__ == "__main__":
    # 1. Automatically generate the list of the 10 JSON files we just created
    # p95_list=['p95lastGBR','p95firstGBR']
    # p95_list=['sorted' , 'sorted_maximum_traffic_1UE_DYNAMIC_rest_STATIC_15MBPS','sorted_maximum_traffic_1UE_STATIC_15MBPS_rest_DYNAMIC','sorted_maximum_traffic_STATIC_15MBPS']
    # p95_list=[ 'sorted_maximum_traffic_1UE_DYNAMIC_rest_STATIC_15MBPS','sorted_maximum_traffic_1UE_STATIC_15MBPS_rest_DYNAMIC','sorted_maximum_traffic_STATIC_15MBPS']
    p95_list=[ 'sorted_maximum_traffic']
    # p95_list=[ 'sorted_maximum_traffic_STATIC']
    
    # scenario_3gbr_3nongbr_4K_
    static_gfbr = np.arange(0, 50_000_001, 5_000_000)
    # json_files = [f"scenario_{i}gbr_{i}nongbr_4K_{p95}.json" for i in range(1, 11) for p95 in p95_list]
    json_files = [f"scenario_{i}gbr_{i}nongbr_4K_{p95}.json" for i in range(1, 11) for p95 in p95_list]
    scenarios = range(1, 11)
    # json_files = [f"scenario_{i}gbr_{i}nongbr_4K_{p95}_{g/1e6:.0f}_MBPS.json" for i in scenarios for p95 in p95_list for g in static_gfbr]
    
    
    # json_files = [f"scenario_{i}gbr_{i}nongbr_4K.json" for i in range(1, 11)]
    # SCHEDULER_TYPES = ['DPP','RR','Qos']
    SCHEDULER_TYPES = ['DPP','RR']

    
    # buffer_sizes = [937500,1250000, 2500000, 3125000,5000000]  # in bytes, corresponds to 1s, 2s, 4s, and 8s of buffering at 1 Gbps
    buffer_sizes = [937500]  # in bytes, corresponds to 1s, 2s, 4s, and 8s of buffering at 1 Gbps

    rng_runs= range(11,101 )  # Run each scenario with 10 different random seeds (rngRun=1 to 10)
    configurations = [(file, scheduler, buffersize, rng_run) for file in json_files for scheduler in SCHEDULER_TYPES for buffersize in buffer_sizes for rng_run in rng_runs]
    # configurations = [(file, scheduler,sim_num) for file in json_files for scheduler in SCHEDULER_TYPES for sim_num in sims]

    # 2. Set the maximum number of parallel simulations.
    # Adjust this based on your hardware! 3 to 4 is usually a safe sweet spot.
    MAX_PARALLEL_RUNS = 40

    print(f"🚀 Starting batch run of {len(json_files)} simulations...")
    print(f"⚙️ Running {MAX_PARALLEL_RUNS} simulations simultaneously.")
    print("-" * 50)

    # 3. Launch the multiprocessing pool
    with concurrent.futures.ProcessPoolExecutor(max_workers=MAX_PARALLEL_RUNS) as executor:
        # This maps our list of files to the run_simulation function
        executor.map(run_simulation, configurations)

    print("-" * 50)
    print("🎉 All 10 simulations have completed!")