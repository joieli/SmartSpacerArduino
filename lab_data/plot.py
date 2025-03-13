import matplotlib.pyplot as plt
import numpy as np
import math
import pandas as pd

from scipy.interpolate import make_interp_spline


time = []
vol_flow = []
pressure = []
lps = []
lps_calc = []
volt = []

iworx_time = []
iworx_air_flow = []
iworx_lung_vol = []
iworx_lps = []


sensor = np.genfromtxt('sensor/offset/forcedin_weak_logging_offset.csv', delimiter=',')
for row in sensor:
    if any(row):
        time.append(float(row[0]) - 3500)
        # vol_flow.append(float(row[1]))
        pressure.append(float(row[2]))
        lps.append(float(row[1]) / 60)  # convert L/min to l/s
        count = float(row[2])/(13108/2000)  # convert pres to count
        if count < 0:
            volt.append(0)
        else:
            volt.append(count)  # 2048 count / 3.3V

iworx = np.genfromtxt('iworx/offset/forcedin_weak_iworx_offset.txt')
for row in iworx:
    iworx_time.append(float(row[0]) * 1000)
    iworx_air_flow.append(float(row[1]))
    iworx_lung_vol.append(float(row[2]))
    if row[3] < 0:
        iworx_lps.append(0)
    else:
        iworx_lps.append(float(row[3]))

for n in volt:
    calc = 4.7257*n+-67.1598*n*n+343.273876*n*n*n+-554.495238*n*n*n*n
    lps_calc.append(calc)

arr = np.array(time)

sensor_spline = make_interp_spline(arr, lps)

x_ = np.linspace(arr.min(), arr.max(), 200)
y_ = sensor_spline(x_)

# plt.plot(time, volt)
plt.scatter(volt, lps)
# plt.scatter(volt, lps_calc)
plt.scatter(iworx_air_flow, iworx_lps)
# plt.scatter(volt, pressure)

plt.xlabel('Voltage')
plt.ylabel('Liters/Seconds')
plt.show(block=True)


# txt_input = r"C:\Users\Karyn Lee\Desktop\hoes\cool\Capstone\iworx\offset\tidal_weak_iworx_offset.txt"   # location of your txt file
# csv_output = r"C:\Users\Karyn Lee\Desktop\hoes\cool\Capstone\iworx\offset\tidal_weak_iworx_offsetCSV.csv"  # location of the csv output

# df = pd.read_csv(txt_input, sep='\t')
# df.to_csv(csv_output, index=False)

