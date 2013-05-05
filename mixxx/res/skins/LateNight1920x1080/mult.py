#!/usr/bin/python

import os, sys, os.path

for fname in sys.argv[1:]:
    f = open(fname)
    
    for line in f.readlines():
        line = line.rstrip()
        if "<POS>" in line.upper():
            s = line.upper().find("<POS>") + 5
            e = line.upper().find("</POS")
            if e == s:
                print line
                continue
            numbers = line[s:e].split(',')
            new_numbers = (str(int(round(int(numbers[0])*1.5))), str(int(round(int(numbers[1])*1.5))))
            print line[0:s] + ','.join(new_numbers) + line[e:]
        elif "<SIZE>" in line.upper():
            s = line.upper().find("<SIZE>") + 6
            e = line.upper().find("</SIZE")
            if e == s:
                print line
                continue
            numbers = line[s:e].split(',')
            try:
                new_numbers = (str(int(round(int(numbers[0])*1.5))), str(int(round(int(numbers[1])*1.5))))
            except:
                print line
                continue
            print line[0:s] + ','.join(new_numbers) + line[e:]    
        else:
            print line   
    

