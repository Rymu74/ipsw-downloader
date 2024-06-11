import requests
import json
from tqdm import tqdm

# URLs
all_devices_url = "https://api.ipsw.me/v4/devices"
overrides_url = "https://rymuios.net/overrides.json"

# List of models to skip
skip_models = {"iPhone1,1", "iPhone1,2", "iPhone2,1", "iPod1,1", "iPod2,1", "iPod3,1"}

print("Fetching list of all device models...")
# Fetch the list of all devices
response = requests.get(all_devices_url)
all_devices = response.json()
print(f"Found {len(all_devices)} device models.\n")

print("Fetching override data...")
# Fetch the override data
response = requests.get(overrides_url)
override_data = response.json()
print("Override data fetched.\n")

# Initialize a list to hold repackaged data for all devices
all_repackaged_data = []

# Insert override data at the beginning of the list
all_repackaged_data.extend(override_data)

# Iterate over each device model with a progress bar
for device in tqdm(all_devices, desc="Processing models", ncols=75, ascii=' >'):
    model = device["identifier"]
    
    # Skip specified models
    if model in skip_models:
        print(f"Skipping model: {model}")
        continue
    
    device_url = f"https://api.ipsw.me/v4/device/{model}"
    
    # Fetch the JSON data for the current model
    response = requests.get(device_url)
    data = response.json()

    # Repackage the data
    for firmware in data["firmwares"]:
        repackaged_data = {
            "model": model,
            "iosVersion": firmware["version"],
            "downloadLink": firmware["url"]
        }
        all_repackaged_data.append(repackaged_data)

print("Finished processing all models.")

# Output the repackaged data to a file
output_file = "repackaged_data.json"
with open(output_file, "w") as f:
    json.dump(all_repackaged_data, f, indent=4)

print(f"Repackaged data written to {output_file}")
