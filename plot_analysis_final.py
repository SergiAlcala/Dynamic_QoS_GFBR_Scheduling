# %%
import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np

# Define the scenarios we simulated
scenarios = range(1, 11)
x_labels = [f"{i}v{i}" for i in scenarios]  # Creates labels like "1v1", "2v2", etc.

total_served_DPP = []
# total_target_DPP = []

total_served_RR = []
# total_target_RR = []

total_served_qos = []
total_served_static = []
total_served_static_15 = []
overall_ratios = []
gbr_ratios = []
nongbr_ratios = []

total_served_DPP_Static_1UE = []
total_served_DPP_Dynamic_1UE = []

# file_dir = "final_results_125mb_buffer"
# file_dir = 'final_results_buffer_937500bps_um_old'
# file_dir = 'final_results_buffer_um_937500_am_3125000'


simulations = range(1, 11)
# buffer = 937500
buffer = 937500
file_dir = f'final_results/buffer_{buffer}bps/'
savefigs= f'FIGS/{file_dir}'
save = True

if not os.path.exists(savefigs) and save:
    os.makedirs(savefigs)

thr_results_dpp = np.zeros((len(scenarios), len(simulations)))
thr_results_rr = np.zeros((len(scenarios), len(simulations)))

# ######### Throughput CALCULATION ################################

print("📊 Analyzing simulation results...")

for i in scenarios:
    for j in simulations:
        # Match the unified CSV name from your TMUX script
        filename_DPP = f"{file_dir}/sim_{j}/unified_stats_scenario_{i}gbr_{i}nongbr_4K_sorted_maximum_traffic_DPP_100.0MHz.csv"
        df = pd.read_csv(filename_DPP)
        df = df[df['Time(s)'] > 0.4]
        df_ue_id_1 = df[df['UE_ID'] == 1]
        normalized_throughput_ue_1 =df_ue_id_1['MeasuredThroughput(Mbps)']
        served_DPP = normalized_throughput_ue_1.mean()
        total_served_DPP.append(served_DPP)
        thr_results_dpp[i-1, j-1] = served_DPP

        filename_RR = f"{file_dir}/sim_{j}/unified_stats_scenario_{i}gbr_{i}nongbr_4K_sorted_maximum_traffic_RR_100.0MHz.csv"
        df_RR = pd.read_csv(filename_RR)
        df_RR = df_RR[df_RR['Time(s)'] > 0.4]
        df_RR_ue_id_1 = df_RR[df_RR['UE_ID'] == 1]
        normalized_throughput_RR = df_RR_ue_id_1['MeasuredThroughput(Mbps)']
        served_RR = normalized_throughput_RR.mean()
        # served_RR = normalized_throughput_RR.quantile(0.95)
        total_served_RR.append(served_RR)
        thr_results_rr[i-1, j-1] = served_RR


    

  

plt.figure(figsize=(10, 6))

# # Plot the three lines
plt.plot(x_labels, thr_results_dpp.mean(axis=1), marker='s', markersize=8, linewidth=2.5, 
         color='tab:blue', label='DPP Dynamic GFBR')
plt.plot(x_labels, thr_results_rr.mean(axis=1), marker='o', markersize=8, linewidth=2.5,
         color='black', linestyle='--', label='Round Robin')


plt.xlabel('Scenario Configuration (GBR UEs vs Non-GBR UEs)')
plt.ylabel('Served Throughput (Mbps)')
# plt.legend(loc=6, fontsize=11)
plt.grid(True, linestyle=':', alpha=0.9)
plt.title('Throughput for UE1 across Scenarios, DPP vs Round Robin',  fontweight='bold')
# Save the plot
plt.tight_layout()
if save:
    plt.savefig(f'{savefigs}/throughput_comparison_{simulations[-1]*2}_UE.png', dpi=300, bbox_inches='tight')
    plt.savefig(f'{savefigs}/throughput_comparison_{simulations[-1]*2}_UE.pdf', dpi=300, bbox_inches='tight')

print('Throughput_DPP:' , thr_results_dpp.mean(axis=1).round(2))
print('Throughput_RR:' , thr_results_rr.mean(axis=1).round(2))


# ######### DROPRATE CALCULATION ################################


total_served_DPP = []
# total_target_DPP = []

total_served_RR = []
# total_target_RR = []

total_served_qos = []
total_served_static = []
total_served_static_15 = []
overall_ratios = []
gbr_ratios = []
nongbr_ratios = []

total_served_DPP_Static_1UE = []
total_served_DPP_Dynamic_1UE = []

dr_results_dpp = np.zeros((len(scenarios), len(simulations)))
dr_results_rr = np.zeros((len(scenarios), len(simulations)))




# # file_dir = "final_results_125mb_buffer"
# # file_dir = 'final_results_buffer_937500bps_um_old'
# file_dir = 'final_results_buffer_um_937500_am_3125000'



# print("📊 Analyzing simulation results...")

for i in scenarios:
    for j in simulations:
        # Match the unified CSV name from your TMUX script
        filename_DPP = f"{file_dir}/sim_{j}/unified_stats_scenario_{i}gbr_{i}nongbr_4K_sorted_maximum_traffic_DPP_100.0MHz.csv"
        df = pd.read_csv(filename_DPP)
        df = df[df['Time(s)'] > 0.4]
        df_ue_id_1 = df[df['UE_ID'] == 1]
        normalized_throughput_ue_1 =df_ue_id_1['MeasuredThroughput(Mbps)'].mean()
        requested_traffic = df_ue_id_1['TargetTraffic(Mbps)'].mean()
        drop = 1 - (normalized_throughput_ue_1 / requested_traffic)
        served_DPP = drop.mean()*100
        total_served_DPP.append(served_DPP)
        dr_results_dpp[i-1, j-1] = served_DPP

        filename_RR = f"{file_dir}/sim_{j}/unified_stats_scenario_{i}gbr_{i}nongbr_4K_sorted_maximum_traffic_RR_100.0MHz.csv"
        df_RR = pd.read_csv(filename_RR)
        df_RR = df_RR[df_RR['Time(s)'] > 0.4]
        df_RR_ue_id_1 = df_RR[df_RR['UE_ID'] == 1]
        normalized_throughput_RR = df_RR_ue_id_1['MeasuredThroughput(Mbps)'].mean()
        requested_traffic_RR = df_RR_ue_id_1['TargetTraffic(Mbps)'].mean()
        drop_RR = 1 - (normalized_throughput_RR / requested_traffic_RR)
        served_RR = drop_RR.mean()*100
        # served_RR = normalized_throughput_RR.quantile(0.95)
        total_served_RR.append(served_RR)
        dr_results_rr[i-1, j-1] = served_RR



    

  

plt.figure(figsize=(10, 6))

# # Plot the three lines
plt.plot(x_labels, dr_results_dpp.mean(axis=1), marker='s', markersize=8, linewidth=2.5, 
         color='tab:blue', label='DPP Dynamic GFBR')
plt.plot(x_labels, dr_results_rr.mean(axis=1), marker='o', markersize=8, linewidth=2.5,
         color='black', linestyle='--', label='Round Robin')


plt.xlabel('Scenario Configuration (GBR UEs vs Non-GBR UEs)')
plt.ylabel('Droprate [%]')
# plt.legend(loc=6, fontsize=11)
plt.grid(True, linestyle=':', alpha=0.9)
plt.title('Droprate for UE1 across Scenarios, DPP vs Round Robin',  fontweight='bold')
plt.tight_layout()
if save:
    plt.savefig(f'{savefigs}/droprate_comparison_{simulations[-1]*2}_UE.png', dpi=300, bbox_inches='tight')
    plt.savefig(f'{savefigs}/droprate_comparison_{simulations[-1]*2}_UE.pdf', dpi=300, bbox_inches='tight')

print('Droprate_DPP:' , dr_results_dpp.mean(axis=1).round(2))
print('Droprate_RR:' , dr_results_rr.mean(axis=1).round(2))


# ######### Delay CALCULATION ################################


print("📊 Analyzing simulation results...")

total_delay_ue1_DPP = []
total_delay_ue1_RR = []
total_delay_ue1_QoS = []
total_delay_ue1_static = []
total_delay_ue1_static_15 = []

gbr_max_value_list= []
gbr_min_value_list= []

RR_max_value_list= []
RR_min_value_list= []

delay_results_dpp = np.zeros((len(scenarios), len(simulations)))
delay_results_rr = np.zeros((len(scenarios), len(simulations)))
quantile_95_dpp = np.zeros((len(scenarios), len(simulations)))
quantile_95_rr = np.zeros((len(scenarios), len(simulations)))
quantile_05_dpp = np.zeros((len(scenarios), len(simulations)))
quantile_05_rr = np.zeros((len(scenarios), len(simulations)))

for i in scenarios:
    for j in simulations:
    # Match the unified CSV name from your TMUX script
        filename_DPP = f"{file_dir}/sim_{j}/unified_stats_scenario_{i}gbr_{i}nongbr_4K_sorted_maximum_traffic_DPP_100.0MHz.csv"
        df = pd.read_csv(filename_DPP)
        df = df[df['Time(s)'] > 0.4]
        df_ue_id_1 = df[df['UE_ID'] == 1]
       

        
        normalized_throughput_ue_1 =df_ue_id_1['MeasuredDelay(ms)'].mean()

        gbr_max_value_list.append(df_ue_id_1['MeasuredDelay(ms)'].quantile(0.95))
        gbr_min_value_list.append(df_ue_id_1['MeasuredDelay(ms)'].quantile(0.05))
        quantile_95_dpp[i-1, j-1] = df_ue_id_1['MeasuredDelay(ms)'].quantile(0.95)
        quantile_05_dpp[i-1, j-1] = df_ue_id_1['MeasuredDelay(ms)'].quantile(0.05)
        
        total_delay_ue1_DPP.append(normalized_throughput_ue_1)
        delay_results_dpp[i-1, j-1] = normalized_throughput_ue_1

        filename_RR = f"{file_dir}/sim_{j}/unified_stats_scenario_{i}gbr_{i}nongbr_4K_sorted_maximum_traffic_RR_100.0MHz.csv"
        df_RR = pd.read_csv(filename_RR)
        df_RR = df_RR[df_RR['Time(s)'] > 0.4]
        df_RR_ue_id_1 = df_RR[df_RR['UE_ID'] == 1]
        delay_ue1_RR = df_RR_ue_id_1['MeasuredDelay(ms)']
        delay_mean_RR = delay_ue1_RR.mean()
        total_delay_ue1_RR.append(delay_mean_RR)
        delay_results_rr[i-1, j-1] = delay_mean_RR

        RR_max_value_list.append(df_RR_ue_id_1['MeasuredDelay(ms)'].quantile(0.95))
        RR_min_value_list.append(df_RR_ue_id_1['MeasuredDelay(ms)'].quantile(0.05))
        quantile_95_rr[i-1, j-1] = df_RR_ue_id_1['MeasuredDelay(ms)'].quantile(0.95)
        quantile_05_rr[i-1, j-1] = df_RR_ue_id_1['MeasuredDelay(ms)'].quantile(0.05)


plt.figure(figsize=(10, 6))
# Plot the three lines
plt.plot(x_labels, delay_results_dpp.mean(axis=1), marker='s', markersize=8, linewidth=2.5, 
         color='tab:blue', label='DPP UE1 Delay')
plt.plot(x_labels, delay_results_rr.mean(axis=1), marker='o', markersize=8, linewidth=2.5,
         color='black', linestyle='--', label='Round Robin UE1 Delay')

plt.fill_between(x_labels, quantile_05_dpp.mean(axis=1), quantile_95_dpp.mean(axis=1), color='tab:blue', alpha=0.1)
plt.fill_between(x_labels, quantile_05_rr.mean(axis=1), quantile_95_rr.mean(axis=1), color='black', alpha=0.1)


plt.xlabel('Scenario Configuration (GBR UEs vs Non-GBR UEs)')

# Add a horizontal dashed line representing a typical 4K video delay limit (e.g., 20ms or 40ms)
plt.axhline(y=150, color='green', linestyle='--', alpha=0.7, label='3GPP 5QI 2 (VIDEO) Delay Budget (150ms)')
plt.ylabel('Delay (ms)' )
# plt.legend(loc=6)
plt.grid(True, linestyle=':', alpha=0.9)
plt.title('Delay for UE1 across Scenarios DPP vs Round Robin',  fontweight='bold')
plt.tight_layout()
if save:
    plt.savefig(f'{savefigs}/delay_comparison_{simulations[-1]*2}_UE.png', dpi=300, bbox_inches='tight')
    plt.savefig(f'{savefigs}/delay_comparison_{simulations[-1]*2}_UE.pdf', dpi=300, bbox_inches='tight')



print('Delay_DPP:' , delay_results_dpp.mean(axis=1).round(2))
print('Delay_RR:' , delay_results_rr.mean(axis=1).round(2))
    


