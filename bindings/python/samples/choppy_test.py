from toolbit import Choppy

choppy = Choppy()
choppy.open()
#choppy.open("00000011")

print("Is connected?", choppy.isConnected())

#print(choppy.showReg())

print(choppy.getColor())

choppy.setColor(1)

print(choppy.getColor())

i=0
while i < 5:
    print(str('%03.3f' % choppy.getVoltage()) + " [V]")
    print(str('%03.3f' % (1000.0 * choppy.getCurrent())) + " [mA]")
    i += 1


# If you want to enable DFU mode to download new firmware, 
# please use the following code: 
# choppy.enableDfu()

