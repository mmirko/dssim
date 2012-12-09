#!/usr/bin/env python2

import os

header=''
chars=''
queries=''
footer=''

header=''

filelist=os.listdir('.')
for filename in filelist:
	if filename.endswith('.lua'):
		function_name=filename[:-4]+'_function'
		f=open(filename,'r')
		chars=chars+'char '+function_name+'[]="\\n\\\n'
		for line in f:
			line=line.replace('<<<CR>>>','\\\\n')
			chars=chars+'\t'+line.rstrip('\n')+' \\n\\\n'
		chars=chars+'\t";\n'
		f.close()

footer=''

#print header
print chars
print queries
#print footer

