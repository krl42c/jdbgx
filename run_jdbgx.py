import os
import sys
import subprocess

if len(sys.argv) < 2: exit("provide a java program to run")
if not os.path.isdir('.jdbgx'): os.mkdir('.jdbgx')

prog = sys.argv[1]

available_ops = ['exit','bp', 'run']

sym_t = open('.jdbg/symbols.jdg', 'w').close()
sym_t = open('.jdbg/symbols.jdg', 'w')

while True:
  opt = input()
  cmds = opt.split(' ')
  r = cmds[0]
  if r not in available_ops: print(f'Unknown command, available commands are: {available_ops}')
  if r == 'bp':
    try: 
      class_name = cmds[1]
      method_name = cmds[2]
      line = cmds[3]
      sym_t.write(f'{class_name}:{method_name}:{line}')
    except IndexError:
      print('Wrong bp format')
  if r == 'exit': break
  if r == 'run': 
    sym_t.close()
    result = subprocess.run(["java", "-agentpath:./agent.so", "-Xdebug", prog], capture_output=True, text=True).stdout.strip("\n")
    print(result)
    break    
