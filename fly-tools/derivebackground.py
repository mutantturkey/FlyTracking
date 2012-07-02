import numpy
import sys
import Image
import scipy
from scipy import stats
if len(sys.argv) != 3:
    print "usage:", sys.argv[0], " filelist-path output-path"

filelistpath = sys.argv[1]
outputpath = sys.argv[2]

filelist = numpy.loadtxt(filelistpath, dtype=numpy.str)
nimages = len(filelist)

temp = numpy.asarray(Image.open(filelist[0]))
height = temp.shape[0]
width = temp.shape[1]

mov = numpy.empty((nimages, height, width, 3), dtype=numpy.uint8)

for i in xrange(nimages):
    mov[i] = numpy.asarray(Image.open(filelist[i]))

bg = scipy.stats.mode(mov, axis=0)

#image.fromarray(numpy.uint8(bg)).save(outputpath)
scipy.misc.imsave(outputpath, bg)

print bg
print "created", outputpath

