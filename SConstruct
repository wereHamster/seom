
import os
import re
import SCons.Defaults
import SCons.Tool
import SCons.Util

env = Environment(
	CC = 'gcc',
	CPPPATH = ['#include'],
	CCFLAGS = ['-std=c99', '-pipe', '-g', '-W', '-Wall'],
	LIBS = ['dl', 'pthread'],
)

src = {
	'common' : [
	],
	'lib' : [
		'src/buffer.c',
		'src/frame.c',
		'src/codec.c',
		'src/stream.c',
		'src/client.c',
		'src/config.c',
		'src/server.c',
		'src/asm/codec.c',
		'src/asm/frame.c',
	],
	'player' : [
		'src/player/colorspace.c',
		'src/player/main.c',
	],
	'server' : [
		'src/server/main.c',
	],
}

machine = os.uname()[4]
if re.match('i.86', machine):
	env['AS'] = 'yasm'
	env['ASFLAGS'] = '-f elf -m x86'
elif machine == 'x86_64':
	env['AS'] = 'yasm'
	env['ASFLAGS'] = '-f elf -m amd64'


static_obj, shared_obj = SCons.Tool.createObjBuilders(env)

shared_obj.add_action('.asm', SCons.Defaults.ASAction)
shared_obj.add_emitter('.asm', SCons.Defaults.SharedObjectEmitter)

objLibrary = env.SharedLibrary('seom', src['lib'] + src['common'])
env.Install('/usr/lib', objLibrary)

objServer = env.Program('seomServer', src['server'] + ['src/server.c'])
env.Install('/usr/bin', objServer)

env = env.Copy()
env.Append(LIBS = ['GL', 'X11', 'seom'])
env.Append(LIBPATH = ['.'])
objPlayer = env.Program('seomPlayer', src['player'])
env.Install('/usr/bin', objPlayer)

objFilter = env.Program('seomFilter', ['src/filter/main.c'] + src['common'])
env.Install('/usr/bin', objFilter)

objExample = env.Program('example', 'example.c')

for file in os.listdir('include/seom'):
	path = os.path.join('include/seom', file)
	if os.path.isfile(path):
		env.Install('/usr/include/seom', path)

env.Default([ objLibrary, objServer, objPlayer, objFilter, objExample ])
env.Alias('install', '/usr')
