####################  MandelBulb animation sequence generator #######################
#Author CallanW ~ www.callanw.com 2015
import os

##editbale options
folderPath = "C:/mandleBulb/sequence"

numberFrames = 5 #generates frames between 1 and numberFrames inclusive
frameMultiplier = .1
gridSize = "400"
power = "8"
mult = "1.5"
maxIter = "1"
sphereMult = "1"
orbitThresh = "0.05"
chunks = "30"
threads = "4"
cval = "-0 1 0"

name = "myMandelBulb"
julia = "off"
dll = "C:/mandleBulb/mandleBulb_new.dll"

####################################################################################################

####################################### FRAME MULTIPLIER MATH ########################################

def frameUpdate(frame):
    #add the value and math you wish to animate below
    return float(maxIter) * float(mult) * (frame)


####################################### DO NOT EDIT BELOW THIS LINE UNLESS YOU KNOW WHAT YOU ARE DOING ########################################

for x in range(1, numberFrames + 1):
    
    maxIter = frameUpdate(x)
    
    curFrame = ""
    
    if(x < 10):
        curFrame = "0%s" %str(x);
    else:
        curFrame = str(x)
    
    #create sequence
    sequenceFile = "mandelbulb.%s.ass" %curFrame
    filePath = os.path.join(folderPath, sequenceFile)
    scriptToWrite = ""


    scriptToWrite = """procedural
    {
     name %s""" %name

    scriptToWrite = scriptToWrite + """\n dso "%s""" %dll + """" """

    scriptToWrite = scriptToWrite + """\n load_at_init on
     matrix 
      1 0 0 0
      0 1 0 0
      0 0 1 0
      0 0 0 1 
     declare gridsize constant INT"""

    scriptToWrite = scriptToWrite + "\n gridsize %s" %gridSize

    scriptToWrite = scriptToWrite + "\n declare max_iter constant INT\n max_iter %s" %maxIter

    scriptToWrite = scriptToWrite + "\n declare power constant FLOAT\n power %s" %power

    scriptToWrite = scriptToWrite + "\n declare spheremult constant FLOAT\n spheremult %s" %sphereMult

    scriptToWrite = scriptToWrite + "\n declare orbitthresh constant FLOAT\n orbitthresh %s" %orbitThresh

    scriptToWrite = scriptToWrite + "\n declare chunks constant INT\n chunks %s" %chunks

    scriptToWrite = scriptToWrite + "\n declare threads constant INT\n threads %s" %threads

    scriptToWrite = scriptToWrite + "\n declare julia constant BOOL\n julia %s" %julia

    scriptToWrite = scriptToWrite + "\n declare Cval constant POINT\n Cval %s" %cval

    scriptToWrite = scriptToWrite + "\n}"

    ##create/open file
    text_file = open(filePath, "w")
    text_file.write(scriptToWrite)
    text_file.close()