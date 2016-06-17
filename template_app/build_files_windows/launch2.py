import os
import subprocess
import time;
import msvcrt as m
def wait():
    m.getch()
conf=open('../release_windows/config.hc','r');
s=conf.read();
conf.close();

conf=open('../release_windows/config.hc','w');
s=s.replace('luke','huike');
conf.write(s);
conf.close();



time.sleep(1);#give game time to initialize and read file

wait();

conf=open('../release_windows/config.hc','r');
s=conf.read();
conf.close();

conf=open('../release_windows/config.hc','w');
s=s.replace('huike','luke');
conf.write(s);
conf.close();




