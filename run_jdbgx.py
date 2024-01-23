import os
import subprocess

result = subprocess.run(["java", "-agentpath:./jdbg_agent.so", "-Xdebug", "Source"], capture_output=True, text=True).stdout.strip("\n")
print(result)