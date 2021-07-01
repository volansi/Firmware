from genericpath import isdir
import os
import serial
import time
import itertools
import subprocess
import argparse
import numpy as np

parser = argparse.ArgumentParser(description='Process tests.')
parser.add_argument('phawk_ser', metavar='f', type=str, nargs='+',
                    help='pixhawk serial port')
parser.add_argument('pracer_ser', metavar='f', type=str, nargs='+',
                    help='pixracer serial port')
args = parser.parse_args()

def convert_to_int(val):
	res = []
	for v in val:
		try:
			ss = int(v)
			res.append(ss)
		except ValueError:
			print(v + " not an int")
			res.append(-1)
	return res

def split_log_element(elements):
	'''{A00000:00000'''
	results_s = []
	results_m = []
	results_l = []
	for e in elements:
		if e[0] is not '{':
			continue
		if len(e) < 2:
			print(e)
			continue
		msg_type = e[1]
		sub_ = e[2:]
		vals = sub_.split(':')
		vals = convert_to_int(vals)
		if msg_type is 'S':
			results_s.append(vals)
		if msg_type is 'M':
			results_m.append(vals)
		if msg_type is 'L':
			results_l.append(vals)

	return results_s,results_m,results_l

def process_results(results):
	if len(results) <1:
		print('No Data')
		return

	ss = results[-1]
	if len(ss) < 2:
		print("unexpected values")
		print(results)
		print(ss)
		return
	small_elements_missing = ss[0] - ss[1]
	print('Total number of messages received ' + str(ss[1]))
	print('Total number of messages sent ' + str(ss[0]))
	print('Delta ' + str(small_elements_missing))
	print("TX Dropped packets (all messages): " + str(pxhawk_log.count("#")))

def write_to_serial(ser,msg):
	print(msg)
	if ser:
		ser.write(bytes(msg + "\n",'utf-8'))

def monitor_px4(ser_1,ser_2,folder_name, wait_for, ignore=False):
	total_msg_1 = ""
	total_msg_2 = ""

	target = time.time() + wait_for
	while time.time() < target:
		b = ser_1.read(1024)
		if len(b) < 1:
			continue
		try:
			total_msg_1 += b.decode("ASCII")
		except:
			pass
		b = ser_2.read(1024)
		if len(b) < 1:
			continue
		try:
			total_msg_2 += b.decode("ASCII")
		except:
			pass

	if not ignore:
		f = os.path.join(folder_name,'pxracer_console.log')
		with open(f,'a') as ff:
			ff.write(total_msg_1)
		f = os.path.join(folder_name,'pxhawk_console.log')
		with open(f,'a') as ff:
			ff.write(total_msg_2)
	return total_msg_1,total_msg_2

def open_serial_port(port):
	BAUDRATE = 57600
	ser = serial.Serial(port, BAUDRATE, timeout=1)  # open serial port
	ser.close()
	ser.open()
	print(ser.name)         # check which port was really used
	return ser


test_set = [
			['PUB_SUB_Test_1',400,-1,-1],
			['PUB_SUB_Test_2',-1,400,-1],
			['PUB_SUB_Test_3',-1,300,-1],
			['PUB_SUB_Test_4',100,100,-1],
			['PUB_SUB_Test_5',200,200,-1],
			['PUB_SUB_Test_6',300,200,-1],
			['PUB_SUB_Test_7',-1,-1,10],
			['PUB_SUB_Test_8',-1,-1,50],
			['PUB_SUB_Test_9',-1,-1,100],
			['PUB_SUB_Test_10',300,100,20],
			['PUB_SUB_Test_11',300,50,50],
			['PUB_SUB_Test_12',200,50,10],
			['PUB_SUB_Test_13',150,25,5],
			['PUB_SUB_Test_14',100,25,1],
			]

for test_item in test_set:
	print("TEST:" + test_item[0])
	pixhawk_ser = open_serial_port(args.phawk_ser[0])
	pixracer_ser = open_serial_port(args.pracer_ser[0])

	folder_name = '/home/shaun/.cantst/' + test_item[0]
	os.system('rm -rf ' + folder_name)
	os.system('mkdir -p ' + folder_name)

	write_to_serial(pixhawk_ser,'uavcan_v1 start')
	write_to_serial(pixhawk_ser,'param set UCAN1_D_PUB_SM 22')
	write_to_serial(pixhawk_ser,'param set UCAN1_D_PUB_MD 23')
	write_to_serial(pixhawk_ser,'param set UCAN1_D_PUB_LG 24')
	write_to_serial(pixhawk_ser,'param set UCAN1_DPUB_SM_HZ '+str(test_item[1]))
	write_to_serial(pixhawk_ser,'param set UCAN1_DPUB_MD_HZ '+str(test_item[2]))
	write_to_serial(pixhawk_ser,'param set UCAN1_DPUB_LG_HZ '+str(test_item[3]))
	write_to_serial(pixhawk_ser,'param save')
	write_to_serial(pixhawk_ser,'reboot')

	write_to_serial(pixracer_ser,'param set UCAN1_DMY1_PID 22')
	write_to_serial(pixracer_ser,'param set UCAN1_DMY2_PID 23')
	write_to_serial(pixracer_ser,'param set UCAN1_DMY3_PID 24')
	write_to_serial(pixracer_ser,'param save')
	write_to_serial(pixracer_ser,'reboot')

	# Clear the pxracer for N seconds while the pixhawk reboots
	monitor_px4(pixracer_ser,pixhawk_ser,folder_name,30,ignore=True)

	# kick off the publishing
	write_to_serial(pixhawk_ser,'uavcan_v1 start')

	pxracer_log, pxhawk_log = monitor_px4(pixracer_ser,pixhawk_ser,folder_name,40)

	pixhawk_ser.close()
	pixracer_ser.close()

	c = pxracer_log.split()
	small,medium,large = split_log_element(c)


	print()
	print("SMALL:")
	process_results(small)


	print()
	print("MEDIUM:")
	process_results(medium)


	print()
	print("LARGE:")
	process_results(large)

