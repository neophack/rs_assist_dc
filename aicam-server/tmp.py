import os
path = os.listdir(os.getcwd())
print (path)

for root, dirs, files in os.walk("."):
        print len(files)


os.system("ls -l ./")
