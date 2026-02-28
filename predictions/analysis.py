import numpy as np
import matplotlib.pyplot as plt

# 1. SETUP FILENAMES
# Replace the suffix with the specific experiment name
suffix = "share-small-BiAtLSTM_0.1_75_30"

filenames = {
    "preds": f"predictions_{suffix}.npy",
    "oracle": f"oracle_values_{suffix}.npy",
    "true_traj": f"true_values_{suffix}.npy",
    "lengths": f"future_lengths_{suffix}.npy",
    "boundaries": f"window_boundaries_{suffix}.npy"
}

# 2. LOAD DATA
preds = np.load(filenames["preds"])        # Model predictions [N]
oracle = np.load(filenames["oracle"])      # Ground truth Max in future [N]
true_traj = np.load(filenames["true_traj"])# Full future trajectory [N, delay]
lengths = np.load(filenames["lengths"])    # Actual valid steps in trajectory [N]
boundaries = np.load(filenames["boundaries"]) # Session start/end indices [NumSessions, 2]

print(f"Loaded {len(preds)} total windows across {len(boundaries)} sessions.")

# 3. CHOOSE A SESSION
# Let's look at the first session
session_idx = 0
start_idx, end_idx = boundaries[session_idx]

# Slice the global arrays to get only this session's data
s_preds = preds[start_idx:end_idx]
s_oracle = oracle[start_idx:end_idx]
s_traj = true_traj[start_idx:end_idx]
s_lens = lengths[start_idx:end_idx]

print(f"\nAnalyzing Session {session_idx}:")
print(f"Indices in global array: {start_idx} to {end_idx}")
print(f"Number of windows in this session: {len(s_preds)}")

# 4. UNDERSTANDING THE DATA STRUCTURE
# Each 'i' represents a single sliding window (one point in time)
sample_i = 10

print(f"\nDetails for Window #{sample_i} inside Session {session_idx}:")
print(f"Model Predicted (Max for next 30s): {s_preds[sample_i]:.2f}")
print(f"Actual Max that occurred: {s_oracle[sample_i]:.2f}")

# The 'true_traj' contains the actual values for the NEXT 'delay' seconds
actual_sequence = s_traj[sample_i]
valid_len = s_lens[sample_i]
actual_sequence_cleaned = actual_sequence[:valid_len]

print(f"The next {valid_len} seconds of raw traffic values were:")
print(actual_sequence_cleaned)

# 5. QUICK VISUALIZATION
plt.figure(figsize=(12, 5))

# Plotting the Predicted vs Oracle Capacity over the whole session
plt.plot(s_oracle, label="Actual Capacity (Oracle)", color='black', linestyle='--')
plt.plot(s_preds, label="Model Prediction", color='blue', alpha=0.7)

plt.fill_between(range(len(s_preds)), s_preds, s_oracle,
                 where=(s_preds < s_oracle), color='red', alpha=0.3, label="Under-estimation")

plt.title(f"Inference Results: Session {session_idx}")
plt.xlabel("Time Steps (Sliding Windows)")
plt.ylabel("Traffic Capacity (Mbps)")
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()

# 6. HOW TO USE THE DATA FOR ANALYSIS
# Example: Calculate total Under-estimation (Cost) for this session
diff = s_preds - s_oracle
under_est_mask = diff < 0
under_est_count = np.sum(under_est_mask)
avg_error = np.mean(np.abs(diff))

print(f"\nSession Analysis Summary:")
print(f"Average Absolute Error: {avg_error:.4f}")
print(f"Under-estimation instances: {under_est_count} out of {len(s_preds)}")
