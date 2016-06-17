import os
import subprocess
import time;

conf=open('config.hc','r');
s=conf.read();
conf.close();

conf=open('config.hc','w');
s=s.replace('luke','huike');
conf.write(s);
conf.close();

os.system('pause');

time.sleep(1);#give game time to initialize and read file

conf=open('config.hc','w');
s=s.replace('huike','luke');
conf.write(s);
conf.close();

subprocess.Popen('game.exe');


