
import os
import SCons.Defaults
import SCons.Tool
import SCons.Util

env = Environment(
	CC = 'gcc',
	CPPPATH = ['#include'],
	CCFLAGS = ['-std=c99', '-pipe', '-O3', '-W', '-Wall'],
	LIBS = ['dl', 'pthread'],
)

if os.uname()[4] == 'ppc':
	machine = 'ppc'
elif os.uname()[4] == 'x86_64':
	env['AS'] = 'yasm'
	env['ASFLAGS'] = '-f elf -m amd64'
	machine = 'amd64'
else:
	env['AS'] = 'yasm'
	env['ASFLAGS'] = '-f elf -m x86'
	machine = 'x86'

static_obj, shared_obj = SCons.Tool.createObjBuilders(env)

shared_obj.add_action('.asm', SCons.Defaults.ASAction)
shared_obj.add_emitter('.asm', SCons.Defaults.SharedObjectEmitter)

srcAssembler = [
	'src/asm/'+machine+'/resample.asm',
	'src/asm/'+machine+'/convert.asm',
	'src/asm/'+machine+'/huffman.asm',
]

srcLibrary = [
	'src/lib/buffer.c',
	'src/lib/config.c',
	'src/lib/codec.c',
	'src/lib/client.c',
]

srcServer = [
	'src/server/main.c',
]

srcPlayer = [
	'src/player/main.c',
	'src/player/colorspace.c',
]

objLibrary = env.SharedLibrary('seom', srcLibrary + srcAssembler)
env.Install('/usr/lib', objLibrary)

objServer = env.Program('seomServer', srcServer)
env.Install('/usr/bin', objServer)

env = env.Copy()
env.Append(LIBS = ['GL', 'X11', 'seom'])
env.Append(LIBPATH = [ '.' ])
objPlayer = env.Program('seomPlayer', srcPlayer)
env.Install('/usr/bin', objPlayer)

objTest = env.Program('test', 'test.c')
objExample = env.Program('example', 'example.c')

for file in os.listdir('include/seom'):
	path = os.path.join('include/seom', file)
	if os.path.isfile(path):
		env.Install('/usr/include/seom', path)

env.Default([ objLibrary, objServer, objPlayer, objExample ])
env.Alias('install', '/usr')
