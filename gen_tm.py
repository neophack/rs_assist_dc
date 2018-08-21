import time


import sys

with open(sys.argv[1], 'wb') as fout:
    while True:
        inp = raw_input()
        if inp != 'e':
            tm = time.time()
            print tm
            print >> fout, tm
        else:
            print 'Program end'
            sys.exit(0)
        

