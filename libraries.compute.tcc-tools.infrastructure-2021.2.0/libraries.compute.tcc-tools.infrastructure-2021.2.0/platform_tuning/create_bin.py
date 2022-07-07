#!/usr/bin/env python3
#/*******************************************************************************
#Copyright 2019 Intel Corporation.
#This software and the related documents are Intel copyrighted materials, and your use of them
#is governed by the express license under which they were provided to you (License).
#Unless the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
#or transmit this software or the related documents without Intel's prior written permission.
#This software and the related documents are provided as is, with no express or implied warranties,
#other than those that are expressly stated in the License.
#
#*******************************************************************************/

from __future__ import print_function
import pandas as pd
import json
import sys
import struct
import sqlite3
import copy
import re
import os

FIND_NAME = r".*\/(.*)\.csv"

EXPECTED_NUM_COLUMNS = 16
HEADER_ROW_INDEX = 9

LAT_INDEX = 2
BW_INDEX = 3
PRODUCER_INDEX = 5 
CONSUMER_INDEX = 7

SKIP_ROWS = 9

if(len(sys.argv) < 2):
	print("Usage: python3 ./create_bin.py <TuningConfigFile> [exit: optional parameter to exit if there are issues.]")
	exit()


csv_file_path = sys.argv[1]
print(csv_file_path)

def create_connection(db_filename):
	try:
		connection = sqlite3.connect(db_filename)
		return connection
	except sqlite3.DatabaseError as err:
		print(err, sys.stderr)
	return None


def create_table(connection, table_name, column_list):
	try:
		create_sqlite3l_req = 'CREATE TABLE IF NOT EXISTS {0} ({1});'.format(table_name, ",".join(column_list))
		curs = connection.cursor()
		curs.execute(create_sqlite3l_req)
		return curs.lastrowid
	except sqlite3.DatabaseError as err:
		print(err, sys.stderr)
	return None


def insert_in_table(connection, table_name, members):
	members_is_list = isinstance(members, list)
	rows = members[0].keys() if members_is_list else members.keys()
	try:
		insert_sqlite3_req = 'INSERT INTO {0}({1}) VALUES({2});'.format(table_name,
																		','.join(rows),
																		','.join(['?' for _ in range(len(rows))]))
		values = [tuple(member.values()) for member in members] if members_is_list else [tuple(members.values())]
		curs = connection.cursor()
		curs.executemany(insert_sqlite3_req, values)
		connection.commit()

		return curs.lastrowid
	except sqlite3.DatabaseError as err:
		print(err, sys.stderr)
	return -1

def create_db():
	conn = create_connection('./tuning.db')

	create_table(conn, 'registers_list', ('id integer PRIMARY KEY NOT NULL',
										  'reg_name text',
										  'reg_type integer',
										  'base integer',
										  'addr integer',
										  'mask text',
										  'port integer',
										  'type integer',
										  'mailbox_type integer',
										  'cmd integer'))

	create_table(conn, 'tuning_configs', ('id integer PRIMARY KEY',
										  'tuning_id varchar(64)',
										  'producer text',
										  'consumer text',
										  'latency integer',
										  'bandwidth integer'))

	create_table(conn, 'tuning_values', ('id integer PRIMARY KEY',
										 'line integer',
										 'tuning_id integer',
										 'reg_id integer NOT NULL',
										 'value integer',
										 'FOREIGN KEY(reg_id) REFERENCES registers_list(id)'))
	return conn

#gets domain based off which unit the register sits in
def domain_check(unit):
	f = unit[0:3]
	if f == 'PSF':
		return "psf"
	return "sa"

#Convert array of values into their respective binary values [8 bytes = 32 bits = 16 nibbles(hex characters)]
#Ex: 7:6 show that the bits at location 6-7[starting index = 0] are 1s, while the others are 0, then convert that to hex as the output
#7:6 in binary is 00000011
#Output for 7:6 would be: 000000C0
def range_to_binary(range_val):
	bin_val = [0]*64
	#Iterate through array of 32 bits of 0s
	for index in reversed(range(0,64)):
		#Set value at index to 1 when index is in given range
		if (index >= (63 - int(range_val[0]))) and (index <= 63 - (int(range_val[1]))):
			bin_val[index] = 1
	return bin_val

#Converts 64 bit(8 byte) binary value(string) to hex(string)
def bin_to_hex(binary):
	hex_add = ""
	for index in range(0,16):
		hex_val = hex(int(binary[index*4:(index+1)*4], 2))
		hex_add += hex_val[2:]
	return hex_add

#Formats hex value to value usable for binary file, given a range
def hex_format(value, range_val):
	hex_address = ""
	#populate hex_address with 0 for given length minus string length of value
	for index in range (0, range_val - len(value)):
		hex_address += "0"
	#concatenate value to address
	hex_address += str(value)
	#final address is of length [range_val]
	return hex_address

#converts the Bus Device and Function values xx:xx:x to a 4 byte hex value (8 words)
def bdf_to_hex(bdf_string):
	bdf_output = ""
	string = bdf_string.split(":")
	if(len(string) < 3) or (string == "TBD"):
		#incoming value doesn't have the correct amount of entries print default
		return "00000000"
	#BDF Caluculation
	#constants
	device = 32
	function = 8
	fun_size = 4096
	dev_size = function*fun_size
	bus_size = dev_size*device
	#values from input
	bus_val = int(string[0])
	dev_val = int(string[1])
	fun_val = int(string[2])
	#calculated values
	bus_out = bus_val*bus_size
	dev_out = dev_val*dev_size
	fun_out = fun_val*fun_size
	bdf_sum = bus_out+dev_out+fun_out
	bdf_hex = hex(bdf_sum)
	if(bdf_sum == 0):
		bdf_output = "00000000"
	else:
		bdf_output =  bdf_hex[2:]+"00"
	return bdf_output


#Checks the register subtype and returns the corresponding byte value
def reg_subtype(string):
	rt_str = ""
	if(string == "None"):
		rt_str = "00"
	elif(string == "MMRd"):
		rt_str = "00"
	elif(string == "MMWr"):
		rt_str = "01"
	elif(string == "IORd"):
		rt_str = "02"
	elif(string == "IOWr"):
		rt_str = "03"
	elif(string == "PCRd"):
		rt_str = "04"
	elif(string == "PCWr"):
		rt_str = "05"
	elif(string == "CRRd"):
		rt_str = "06"
	elif(string == "CRWr"):
		rt_str = "07"
	elif(string == "Cmp"):
		rt_str = "20"
	elif(string == "Cmp0"):
		rt_str = "21"
	else:
		raw_input("Unexpected register subtype given. Press any key to exit...")
		sys.exit()
	return rt_str


class _mmio32:
	def __init__(self, u32_base, u32_addr, u32_mask, u32_data):
		self.u32_base = u32_base  # ECAM format B:D:F:R of BAR
		self.u32_addr = u32_addr  # offset from BA
		self.u32_mask = u32_mask  # data bit-mask
		self.u32_data = u32_data  # data value

	def get_fields_list(self):
		return [(self.u32_base, 32), (self.u32_addr, 32), (self.u32_mask, 32), (self.u32_data, 32)]

class _msr:
	def __init__(self, u32_addr, u64_mask, u64_data):
		self.u32_addr = u32_addr  # ECX value
		self.u64_mask = u64_mask  # EDX:EAX data bit-mask
		self.u64_data = u64_data  # EDX:EAX data value

	def get_fields_list(self):
		return [(self.u32_addr, 32), (self.u64_mask, 64), (self.u64_data, 64)]


class _iosfsb:
	def __init__(self, u8_port, u8_type, u32_addr, u32_mask, u32_data):
		self.u8_port = u8_port  # IOSFSB Port ID
		self.u8_type = u8_type  # IOSFSB Register Type (command)
		self.u32_addr = u32_addr  # register address
		self.u32_mask = u32_mask  # data bit-mask
		self.u32_data = u32_data  # data value

	def get_fields_list(self):
		return [(self.u8_port, 8), (self.u8_type, 8), (self.u32_addr, 32), (self.u32_mask, 32), (self.u32_data, 32)]

class _mailbox:
	def __init__(self, m_type, u32_cmd, u32_mask, u32_data):
		self.m_type = m_type  # Mailbox Register Type
		self.u32_cmd = u32_cmd  # register command
		self.u32_mask = u32_mask  # data bit-mask
		self.u32_data = u32_data  # data value

	def get_fields_list(self):
		return [(self.m_type, 32), (self.u32_cmd, 32), (self.u32_mask, 32), (self.u32_data, 32)]


class TCC_REGISTERS:
	def __init__(self, e_type, info):
		self.e_type = e_type
		self.info = info

	def write(self, file):
		format = 'I'
		max_sizeof = 160
		sizeof_list = lambda flist: sum([elem[1] for elem in flist])
		fields_list = self.info.get_fields_list()
		b_list = bytes()
		b_list += struct.pack(format, self.e_type)
		types = {'B': 8, 'H': 16, 'I': 32, 'Q': 64}
		for field in fields_list:
			format = list(types.keys())[list(types.values()).index(field[1])]
			b_list += struct.pack(format, field[0])

		diff = (max_sizeof - sizeof_list(fields_list)) // 8
		for i in range(0, diff):
			b_list += struct.pack('B', 0)
		try:
			file.write(bytes([*b_list]))
		except IOError as err:
			print("Couldn't write to file (%s)." % err)


class Database:
	def __init__(self, name):
		self.name = name
		try:
			self.connection = sqlite3.connect(self.name)
		except sqlite3.DatabaseError as err:
			print(err, sys.stderr)
			self.connection = None

	# TODO: 'select' must use prepared statement
	def select(self, sql_expression):
		curs = self.connection.cursor()
		sql_select_req = sql_expression
		try:
			curs.execute(sql_select_req)
			return curs.fetchall()
		except sqlite3.DatabaseError as err:
			print(err, sys.stderr)


class DatabaseOperations:
	def __init__(self, db):
		self.db = db

	def select_reg(self, reg_name,  tuning_id):
		raw_select = self.db.select(
			'SELECT  registers_list.base, registers_list.addr, registers_list.mask, tuning_values.value FROM registers_list INNER JOIN tuning_values '
			'ON tuning_values.reg_id == registers_list.id WHERE registers_list.reg_name == "{0}" AND tuning_values.tuning_id == {1}'.format( reg_name, tuning_id))
		if not raw_select:
			return -1
		# print(raw_select)
		return raw_select


	def select_config(self, spec, last_tuning_id):
		# spec_converter = RequirementsConverter(spec)
		converted_spec = spec # spec_converter.get_db_spec() #TODO fix
		if last_tuning_id != "":
			raw_select = self.db.select(
				'SELECT tuning_id FROM tuning_configs WHERE latency<={0} AND bandwidth>={1} '
				'AND producer="{2}" AND consumer="{3}" '
				'AND latency < (SELECT latency FROM tuning_configs WHERE tuning_id = {4})'
				'ORDER BY latency DESC'
				.format(converted_spec.requirementsSpecification.requirements[0].latency,
						converted_spec.requirementsSpecification.requirements[0].bandwidth,
						converted_spec.requirementsSpecification.requirements[0].producer,
						converted_spec.requirementsSpecification.requirements[0].consumer,
						last_tuning_id))
		else:
			raw_select = self.db.select(
				'SELECT tuning_id FROM tuning_configs WHERE latency<={0} AND bandwidth>={1} '
				'AND producer="{2}" AND consumer="{3}" '
				'ORDER BY latency DESC'
				.format(converted_spec.requirementsSpecification.requirements[0].latency,
						converted_spec.requirementsSpecification.requirements[0].bandwidth,
						converted_spec.requirementsSpecification.requirements[0].producer,
						converted_spec.requirementsSpecification.requirements[0].consumer))

		if not raw_select:
			return -1
		tun_ids = [tuning[0] for tuning in raw_select]
		return int(tun_ids[0])

	def __parameterized_select_config(self, latency, bandwidth, last_tuning_id=None, producer=None, consumer=None):
		sql_select_expression = ('SELECT tuning_id FROM tuning_configs WHERE latency<={0} AND bandwidth>={1} '
								 .format(latency, bandwidth))

		if last_tuning_id is not None:
			sql_select_expression += ('AND latency < (SELECT latency FROM tuning_configs WHERE tuning_id={0})'
									  .format(last_tuning_id)) + ' '
		if producer is not None:
			sql_select_expression += ('AND producer={0}'.format(producer)) + ' '
		if consumer is not None:
			sql_select_expression += ('AND consumer={0}'.format(consumer)) + ' '

		sql_select_expression += 'ORDER BY latency DESC'

		raw_select = self.db.select(sql_select_expression)
		if not raw_select:
			return -1
		tun_ids = [tuning[0] for tuning in raw_select]
		return int(tun_ids[0])


class RequirementsConverter:
	def __init__(self, input_spec):
		self.input_spec = input_spec

	def get_db_spec(self):
		prod_cons_regex = re.compile(r'[a-fA-F0-9]+:[a-fA-F0-9]+:[a-fA-F0-9]+:[0-9]+$')
		tmp_spec = copy.deepcopy(self.input_spec)
		for req in tmp_spec.requirementsSpecification.requirements:
			if re.match(prod_cons_regex, req.producer):
				producer = re.findall(prod_cons_regex, req.producer)[0].split(':')
				req.producer = "Core0"
			if re.match(prod_cons_regex, req.consumer):
				consumer = re.findall(prod_cons_regex, req.consumer)[0].split(':')
				req.consumer = "PCIe Root Complex 0"
		return tmp_spec

def get_next_tun_id(spec, conf):
	db_file = './tuning.db'
	db = Database(db_file)
	req_num = len(spec.requirementsSpecification.requirements)
	if req_num != 1:  # only supporting 1 input requirements set
		print("req_num = {0}".format(req_num))
		exit(-1)

	db_ops = DatabaseOperations(db)

	tun_id = db_ops.select_config(spec, conf.get_tun_id())
	return tun_id

def get_value_for_apply_config(reg_name, tuning_id):
	db_file = './tuning.db'
	db = Database(db_file)
	db_ops = DatabaseOperations(db)
	result = db_ops.select_reg(reg_name, tuning_id)
	return  result

def create_bin( bin_path, tun_id):
	db_file = './tuning.db'
	db = Database(db_file)
	raw_select = db.select('SELECT reg_id,value FROM tuning_values WHERE tuning_id={0}'.format(tun_id))
	MvpRtInfo = list()
	for reg_id, value in raw_select:
		record = db.select('SELECT * FROM registers_list WHERE id={0}'.format(reg_id))[0]
		if record[1] == 'mmio32':
			MvpRtInfo.append(TCC_REGISTERS(record[2], _mmio32(record[3], record[4], int(record[5], 16), value)))
		elif record[1] == 'mailbox':
			MvpRtInfo.append(TCC_REGISTERS(record[2], _mailbox(record[8], record[9], int(record[5], 16), value)))

		elif record[1] == 'msr':
			MvpRtInfo.append(TCC_REGISTERS(record[2], _msr(record[4], int(record[5], 16), value)))

		elif record[1] == 'iosfsb':
			MvpRtInfo.append(
				TCC_REGISTERS(record[2], _iosfsb(record[6], record[7], record[4], int(record[5], 16), value)))
			#break;
	try:
		with open(bin_path, "wb") as file:
			for elem in MvpRtInfo:
				elem.write(file)
	except IOError as err:
		print("Couldn't open or write to file (%s)." % err)

header_index = dict()
headers = ['Long Name', 'Register Name', 'Unit', 'Type', 'CR Port ID', 'IOSFSB Subtype', 'Mailbox Type', 'Mailbox CMD', 'Mailbox Parameter', 'BB:DD:F', 'Offset', 'Bit Range Name', 'Bits', 'Value (Hex)', 'Desc', 'Message']

def init_header(): 
	i = 0
	for header in headers:
		header_index[header] = i
		i = i + 1

def check_header(data, header_name):
	"""
	Return 1 if the header according to the spec else return 0.

	"""

	if(data[header_index[header_name]][HEADER_ROW_INDEX] != header_name):
		return 0	

	return 1

def check_headers(data):
	"""
	Return 1 if the headers are according to the spec else return 0.

	"""

	for header in headers:
		if(not check_header(data, header)):
			return 0

	return 1

def check_metadata(data):
	"""
	Return 1 if meta data is according to the spec else return 0.

	"""

	returnVal = 1;

	if(data.shape[1] != EXPECTED_NUM_COLUMNS):
		print("Not a valid config file. It has " + str(data.shape[1]) + " columns, instead of " + str(EXPECTED_NUM_COLUMNS) + " columns.")
		returnVal = 0


	for index in range(0,9):
		if(data[0][index] == 'Long Name'):
			print("Not a valid config file. Metadata has only " + str(index) + " fields.\n")
			returnVal = 0

	if(not check_headers(csv_file)):
		print("Headers are not according to the spec. Expected order: ")
		print("###########################################################################")
		print(','.join(headers))
		print("###########################################################################")
		returnVal = 0

	return returnVal
	

#convert stored data to string to json and stored in a JSON object
if __name__ == '__main__':

	init_header()

	conn = create_db()
	_id = 1
	tun_id = 1
	csv_file = pd.DataFrame(pd.read_csv(csv_file_path, header=None))

	if(not check_metadata(csv_file)):
		# if second argument is provided and is exit, exit without continuing
		if(len(sys.argv) > 2 and sys.argv[2] == "exit"):
			exit()

	latency = float(csv_file[0][LAT_INDEX].replace(" us", ""))
	bandwidth = float(csv_file[0][BW_INDEX].replace(" MB/s", ""))
	producer = csv_file[0][PRODUCER_INDEX]
	consumer = csv_file[0][CONSUMER_INDEX]

	config_members = {'tuning_id': tun_id, 'producer': producer, 'consumer': consumer, 'latency': latency, 'bandwidth': bandwidth}
	insert_in_table(conn, 'tuning_configs', config_members)

	registers_members = []
	registers_members1 = []
	registers_members2 = []
	registers_members3 = []
	tuning_values = []
	line =1
	csv_file = pd.DataFrame(pd.read_csv(csv_file_path, skiprows=SKIP_ROWS))
	#print(csv_file)
	n = csv_file.to_json()
	#print("#########################################")
	container = json.loads(n)
	r_type = container['Type']
	bdf = container['BB:DD:F']
	offset = container['Offset']
	b_range = container['Bits']
	p_id = container['CR Port ID']
	name = container['Register Name']
	iosf_sb = container['IOSFSB Subtype']
	# mb_type = container['Mailbox Type']
	mb_cmd = container['Mailbox CMD']
	mb_param = container['Mailbox Parameter']
	val = container['Value (Hex)']

	#instantiate arrays
	addr = {}
	base = {}
	mask = {}
	port = {}
	mailbox_type = {}
	cmd = {}
	value = {}
	#populate arrays with values and convert values to hex values for binary format
	for x in range(len(val)):
		y = str(x)
		bin_array = range_to_binary(str(b_range[y]).split(":"))
		shift_val = bin_array[-1::-1].index(1) #Find position for first "1" in list for shifted value
		mask[x] = bin_to_hex("".join(str(e) for e in bin_array))
		val[y] = val[y]
		value[x] = (int(val[y], 16)<< shift_val)
		addr[x] = offset[y]


		reg_id = 0
		if(r_type[y] == "MMIO32"):
			base[x] = bdf_to_hex(bdf[y])
			reg_id = _id
			registers_members.append({'id': reg_id,
							   'reg_name': 'mmio32',
							   'reg_type': 0,
							   'base': 4275109888,  #int(base[x], 16),
							   'addr': int(addr[x], 16),
							   'mask': hex(int(mask[x], 16))})
			#print(int(mask[x], 16))
		elif(r_type[y] == "MSR"):
			reg_id = _id
			registers_members3.append({'id': reg_id,
							   'reg_name': 'msr',
							   'reg_type': 1,
							   'addr': int(addr[x], 16),
							   'mask': hex(int(mask[x], 16))})
		elif(r_type[y] == "IOSFSB"):
			port[x] = p_id[y]
			reg_id = _id
			registers_members1.append({'id': reg_id,
							   'reg_name': 'iosfsb',
							   'reg_type': 2,
							   'port': int(port[x], 16),
							   'type': 0, #TODO: Use real type
							   'addr': int(addr[x], 16),
							   'mask': hex(int(mask[x], 16))}
							)
		elif(r_type[y] == "Mailbox"):
			reg_id = _id
			# mailbox_type[x] = mb_type[y]
			#Mailbox Parameter should be added before CMD
			cmd[x] = mb_param[y][2:] + mb_cmd[y][2:]
			registers_members2.append({'id': reg_id,
							  'reg_name': 'mailbox',
							  'reg_type': 3,
							  'mailbox_type': 1,#int(mailbox_type[x], 16),
							  'cmd': int(cmd[x], 16),
							  'mask': hex(int(mask[x], 16))}
							)
		tuning_values.append({'id': _id, 'tuning_id': str(tun_id), 'line': line, 'reg_id': reg_id, 'value': value[x]})
		_id += 1
		line += 1
	if registers_members:
		insert_in_table(conn, 'registers_list', registers_members)
	if registers_members1:
		insert_in_table(conn, 'registers_list', registers_members1)
	if registers_members2:
		insert_in_table(conn, 'registers_list', registers_members2)
	if registers_members3:
		insert_in_table(conn, 'registers_list', registers_members3)
	insert_in_table(conn, 'tuning_values', tuning_values)
	conn.close()
	mvprt_bin = "{0}/MVPRT.bin".format(os.path.dirname(os.path.abspath(__file__)))
	create_bin(mvprt_bin, '1')
