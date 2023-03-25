#!/usr/bin/env python3
import json
import os
from PIL import Image
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path
import soundfile as sf
import matplotlib.pyplot as plt
import samplerate

#returns tuple (name, height, width, payload)
#Serialization order is left-to-right, top-to-bottom, so for the eInk
#display all images have to be rotated 90 degrees counterclockwise
def make_wav_payload(file_path):
    print("Processing WAV file %s" % (file_path))
    payload = []
    data_in, datasamplerate = sf.read(file_path)
    # This means stereo so extract one channel 0
    if len(data_in.shape)>1:
        data_in = data_in[:,0]

    converter = 'sinc_fastest'  # or 'sinc_fastest', ...
    desired_sample_rate = 44000.0
    ratio = desired_sample_rate/datasamplerate
    data_out = samplerate.resample(data_in, ratio, converter)
    maxValue = max(data_out)
    minValue = min(data_out)
    vrange = (maxValue - minValue) 
    firstvalue = 0
    lastvalue = 0
    for v in data_out:
        # scale v to between 0 and 1
        isin = (v-minValue)/vrange   
        v =  int((isin * 255))
        if (firstvalue==0):
            firstvalue= v
        lastvalue = v
        payload.append(v)
    end_value = int( (firstvalue + lastvalue) / 2)        
    payload.append(end_value)
    return payload

def make_binary_payload(file_path):
    payload = []
    print("Processing binary file %s" % (file_path))
    with open(file_path, mode="rb") as bin_file:
        while d:= bin_file.read(1):
            payload.append(int.from_bytes(d,'big'))

    return payload
def make_image_array(file_path, rotate = False, forFont = False):
    im = Image.open(file_path)
    if rotate:
        im = im.rotate(rotate, expand=True)
    width, height = im.size
    print("Processing file %s color depth is %s, width %d, height %d" % (file_path, im.mode, width, height))
    background = Image.new("RGB", im.size, (0,0,0))
    background.paste(im, mask=im.split()[3]) # 3 is the alpha channel
    rgb_im = background.convert('L' if forFont else 'RGB') #make sure the color depth is 1-bit
    # rgb_im.save(file_path+'.png')

    payload = []

    for y in range(0,height):
        for x in range(0,width):
            pixel = rgb_im.getpixel((x,y))
            if pixel:
                 if forFont:
                     payload.append(pixel)
                 else:
                     payload.append(((pixel[0] >> 3) << 11) | ((pixel[1] >> 2) << 5) | (pixel[2] >> 3))
            else:
                if forFont:
                     payload.append(pixel)

    return (width, height, payload)

def serialize_binary(name, payload, forsnd):
    header_output = ('extern const uint8_t %s[];\n' % name,'extern const sound_data_t *%s;\n' % name)[forsnd]
    output = 'const uint8_t %s%s[] = {\n' % (("","_")[forsnd],name)
    row = ''
    c = 0;
    for byte in payload:
        row += '0x%02X,' % byte
        c += 1
        if c > 9:
            c = 0
            row += '\n'
    row = row[0:-1] #get rid of last comma
    output += row
    output += '};\n'
    output += ('\n','const sound_data_t *%s=(sound_data_t *)_%s;\n' % (name,name))[forsnd]
    return (header_output, output)

def serialize_image_array(name, height, width, payload, forFont=False):
    header_output = ('extern const bitmap_t %s;\n' % name, '')[forFont]
    output = '\nstatic const %s %s_payload[] ={\n' % (("uint16_t","uint8_t")[forFont],name)
    row = ''
    c = 0;
    for byte in payload:
        row += ('0x%04X,','0x%02X,')[forFont] % byte
        c += 1
        if c > 9:
            c = 0
            row += '\n'
    row = row[0:-1] #get rid of last comma
    row += '};\n\n'
    output += row
    output += 'const %s %s%s = {.size_x=%d, .size_y=%d, .payload=%s_payload};\n' % (("bitmap_t","bitmap_font_t")[forFont],("","BITMAP_")[forFont],name, width, height, name)
    return (header_output, output)
    

def make_font_description(source_path,file_name,rotation):
    json_file_path = source_path+file_name
    print('Making font description from %s' % json_file_path)
    input_description = json.loads(open(json_file_path).read())
    name = os.path.splitext(file_name)[0]
    pngfile = input_description['config']['textureFile']
    height = input_description['config']['textureHeight']
    counter = len(input_description['symbols'])
    output_header = 'extern const font_t %s;\n' % name
    output_data = ''
    output_data += 'static const glyph_t FONT_GLYPHS_%s[] = {\n' % name
    counter = 0
    for g in input_description['symbols']:
        output_data += '{ .character=%d/*%c*/, .width=%d, .x_offset=%d },\n' % (g['id'], g['id'], g['width'], g['x'])
        counter += 1
    output_data += '};\n\n'

    output_data += '\nextern const bitmap_font_t BITMAP_%s;\nconst font_t %s = { .bitmap = &BITMAP_%s, .glyph_count = %d, .glyphs = FONT_GLYPHS_%s };\n\n' % (name, name, name, counter, name)

    print('Reading payload glyphs: %s%s' % (source_path,pngfile))


    width, height, payload = make_image_array(source_path+pngfile, rotation, True)
    header_output, data  = serialize_image_array(name, height, width, payload,True)
    output_header += header_output;
    output_data   += data;

    return (output_header, output_data)

def make_binary_entry(source_path,file_name,forsnd):
    payload = make_binary_payload(source_path+file_name)
    name = os.path.splitext(file_name)[0]
    header_output, data  = serialize_binary(name, payload,forsnd)

    return (header_output, data)


def make_image_entry(source_path,file_name,rotation):
    output_data = []
    width, height, payload = make_image_array(source_path+file_name, rotation)
    name = os.path.splitext(file_name)[0]

    header_output, data  = serialize_image_array(name, height, width, payload,False)

    return (header_output, data)

def make_wav_entry(source_path,file_name):
    payload = make_wav_payload(source_path+file_name)
    name = os.path.splitext(file_name)[0]
    header_output, data  = serialize_binary(name, payload, False)
    header_output += "#define "+name.upper()+"_LEN "+str(len(payload))+"\n\n"


    return (header_output, data)
#----------------- main -----------------
try:
    parser = argparse.ArgumentParser(description='OpenHMI Resource Builder', prog="ohrc")
    parser.add_argument('-l', '--list', metavar='listfile',nargs='?', help='list containing resource files (json & png)')    
    parser.add_argument('-d', '--dest', metavar='destpath',help='destination path where the resource bin and flash layout will be written')    
    parser.add_argument('file', nargs='*', help='list of files appended after the ones in the listfile')    
    args = parser.parse_args()
    if (args.list == None and len(args.file) == 0):
        parser.error("please provide either file with files list or list of files in the command line");
        sys.exit(1)
    files = []
    if args.list != None : 
        with open(args.list, "r") as flist: 
            files = [line.rstrip() for line in flist]

    files.extend(args.file)

    destination_path = args.dest

except Exception as e:
    print(str(e))
    sys.exit(1)


rotation = 0
addr = 0

output_c_file = open(destination_path+'/resource.c', 'w')
output_h_file = open(destination_path+'/resource.h', 'w')
processed_files = []
output_h_file.write('#ifndef __POP2040_RES_H__\n#define __POP2040_RES_H__\n\n#include "timage.h"\n#include "tmusic.h"\n\n')
output_c_file.write('#include "resource.h"\n\n')


for filepath in files:
    file_path, file_name = os.path.split(filepath)
    if file_path == '':
        file_path = '.'
    file_path += '/'
    if filepath.endswith(".json"):
        header, data = make_font_description(file_path,file_name,rotation)
        processed_files.append(filepath)
        output_h_file.write(header)
        output_c_file.write(data)
    if filepath.endswith(".png"):
        header, data = make_image_entry(file_path,file_name,rotation)
        processed_files.append(filepath)
        output_h_file.write(header)
        output_c_file.write(data)
    if filepath.endswith(".bin"):
        header, data = make_binary_entry(file_path,file_name,False)
        output_h_file.write(header)
        output_c_file.write(data)
    if filepath.endswith(".pwm"):
        header, data = make_binary_entry(file_path,file_name,True)
        output_h_file.write(header)
        output_c_file.write(data)
    if filepath.endswith(".wav"):
        header, data = make_wav_entry(file_path,file_name)
        output_h_file.write(header)
        output_c_file.write(data)
        # Read the file an print binary
        


output_h_file.write('\n#endif /* ! __POP2040_RES_H__ */\n')
