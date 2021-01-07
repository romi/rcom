from registry import *

registry = Registry()
oquam = registry.connect("oquam", "cnc", 60.0)
oquam.execute("homing")



