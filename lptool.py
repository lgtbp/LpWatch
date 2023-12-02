#!/usr/bin/env python
# -*- coding: UTF-8 -*-

import os
import getopt
import sys
import zipfile
import base64
import json
from Crypto.Signature import PKCS1_v1_5 as pk
from Crypto.Hash import SHA256
from Crypto.PublicKey import RSA
import decimal

VERSION = '0.1.0'

def bytesToHexString(bs):
    return ''.join(['%02X' % b for b in bs])

def file_sign_ca2(src_path, pri_path, ca_flg=0):
    print("sign " + src_path)
    fsrc = ""

    with open(src_path, "rb") as x:
        fsrc = x.read()
    print("sha256:")
    print(SHA256.new(fsrc).hexdigest())

    signer = pk.new(RSA.importKey(open(pri_path, 'r').read()))
    signstr = signer.sign(SHA256.new(fsrc))

    print("sign:")
    print(bytesToHexString(signstr))
    jdata = {}
    data = json.loads(json.dumps(jdata))
    data['sign'] = str(base64.b64encode(signstr), "utf8")
    data['ca'] = ca_flg
    jstr = bytes(json.dumps(data), 'utf-8')
    print(jstr)
    return jstr

def deal_pack_bin_version(i18n_value, out_path, version):
    print("in: " + i18n_value + ",out: " +
          out_path + ",version:" + str(version))
    fsrc = ""
    with open(i18n_value, "rb") as x:
        fsrc = x.read()

    fsrc_new = bytearray(fsrc)
    fsrc_new[44] = (version >> 24) & 0xff
    fsrc_new[45] = (version >> 16) & 0xff
    fsrc_new[46] = (version >> 8) & 0xff
    fsrc_new[47] = version & 0xff

    with open(out_path, "wb") as x:
        x.write(fsrc_new)

def deal_pack_app(i18n_value, out_path, version, path_ca="LpTestCa.key", path_crt="NaNo"):
    py_path = os.path.dirname(os.path.realpath(__file__))  # 本py路径
    i18n_out_path = py_path + "\\tool\\" + out_path
    i18n_value = py_path + "\\" + i18n_value
    print("out_path : " + i18n_out_path)
    print("in_path : " + i18n_value)

    deal_pack_bin_version(i18n_value, "build/app.bin", version)

    zip_file = zipfile.ZipFile(i18n_out_path, 'w', zipfile.ZIP_DEFLATED)
    zip_file.write("build/app.bin", "app.bin")

    ca_flg = 0
    if path_crt != "NaNo":
        zip_file.write(path_crt, "ca")
        ca_flg = 1
    info = zip_file.getinfo('app.bin')
    info.comment = file_sign_ca2("build/app.bin", path_ca, ca_flg)
    zip_file.close()

def returnFloat(arg):
    new_arg = int(arg)
    result = decimal.Decimal(str(arg)) - decimal.Decimal(str(new_arg))
    result = float(result)
    return result

def deal_rand_gps(i18n_value, out_path):
    py_path = os.path.dirname(os.path.realpath(__file__))  # 本py路径
    i18n_out_path = py_path + "\\Debug\\" + out_path
    print("out_path : " + i18n_out_path)
    print("rand num : " + i18n_value)

    str_out = ""

    for i in range(int(i18n_value)):

        n_off = 22.536492 + i * 0.00002
        e_off = 113.929381 + i * 0.00002   # 一次平移2米

        tn_off = returnFloat(n_off) * 60
        te_off = returnFloat(e_off) * 60

        str_one = "$GPRMC,081836,A," + str(int(n_off)) + f"{tn_off:.4f}" + \
            ",N," + str(int(e_off)) + f"{te_off:.4f}" + \
            ",E,000.0,360.0,130998,011.3,E*00\n"
        str_out += str_one

    with open(i18n_out_path, "w") as x:
        x.write(str_out)
 
def myAlign(string, length=16):
    if length == 0:
        return string
    slen = len(string)
    re = string
    if isinstance(string, str):
        placeholder = ' '
    else:
        placeholder = u'　'
    while slen < length:
        re += placeholder
        slen += 1
    return re

def print_help():
    print("lptool help:")
    print(myAlign("-v") + "version " + VERSION)
    print(myAlign("-h") + "help")
    # print(myAlign("-o") + "out file path")
    print(myAlign("--RandGps") + "out rand gps data:gps num")
    print(myAlign("--Version") + "add app Version:0,1,2")
    print(myAlign("--Ca") + "ca key:LpTestCa.key")
    print(myAlign("--Crt") + "ca Crt:test.crt")
    print(myAlign("--PackAppTest") + "ca sign app path:build/lwatch.bin")
 
def app_main():
    app_cr_path = "LpTestCa.key"
    app_crt_path = "NaNo"
    version = 0
    out_path = "out.bin"
    opts, args = getopt.getopt(sys.argv[1:], "vho:", ["RandGps=", "Version=", "Ca=", "Crt=", "PackAppTest="])
    for name, value in opts:
        print("input ", name + "," + value)
        if name in ("--RandGps"):
            deal_rand_gps(value, out_path.replace(".bin", ".txt"))
            exit()
        elif name == ("--Ca"):
            app_cr_path = value
        elif name == ("--Crt"):
            app_crt_path = value
        elif name == ("--Version"):
            version = int(value)
        elif name == ("--PackAppTest"):
            deal_pack_app(value, out_path, version, app_cr_path, app_crt_path)
            exit()
        elif name in ("-o"):
            out_path = value
        elif name in ("-h"):
            print_help()
            exit()
        elif name in ("-v"):
            print("-v \t version "+VERSION)
            exit()

if __name__ == '__main__':
    app_main()
    print_help()

# lptool.py -o gps.txt --RandGps=10000 输出1万条gps路径数据到路径 ./Debug
# lptool.py -o app.zip --PackAppTest=build/lwatch.bin 使用默认路径LpTestCa.key对lwatch.bin签名-输出到tool路径
# lptool.py -o app.zip --Ca=LpTestCa.key --Crt=test.crt --PackAppTest=build/lwatch.bin 指定ca