
import os
import re
import SCons.Defaults
import SCons.Tool
import SCons.Util

env = Environment(
	CC = 'gcc',
	CPPPATH = ['#include'],
	CCFLAGS = ['-std=c99', '-pipe', '-O3', '-W', '-Wall'],
	LIBS = ['dl', 'pthread'],
)

src = {
	'common' : [
	],
	'lib' : [
		'src/lib/buffer.c',
		'src/lib/client.c',
		'src/lib/codec.c',
		'src/lib/config.c',
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
if re.match('i?86', machine):
	env['AS'] = 'yasm'
	env['ASFLAGS'] = '-f elf -m x86'
	
	src['common'].append([
		'src/common/x86/resample.asm',
		'src/common/x86/convert.asm',
		'src/common/x86/huffman.asm',
	])
elif machine == 'x86_64':
	env['AS'] = 'yasm'
	env['ASFLAGS'] = '-f elf -m amd64'
	
	src['common'].append([
		'src/common/amd64/resample.asm',
		'src/common/amd64/convert.asm',
		'src/common/amd64/huffman.asm',
	])
else:
	src['common'].append([
		'src/common/resample.c',
		'src/common/convert.c',
		'src/common/huffman.c',
	])

static_obj, shared_obj = SCons.Tool.createObjBuilders(env)

shared_obj.add_action('.asm', SCons.Defaults.ASAction)
shared_obj.add_emitter('.asm', SCons.Defaults.SharedObjectEmitter)

objLibrary = env.SharedLibrary('seom', src['lib'] + src['common'])
env.Install('/usr/lib', objLibrary)

objServer = env.Program('seomServer', src['server'])
env.Install('/usr/bin', objServer)

env = env.Copy()
env.Append(LIBS = ['GL', 'X11', 'seom'])
env.Append(LIBPATH = ['.'])
objPlayer = env.Program('seomPlayer', src['player'])
env.Install('/usr/bin', objPlayer)

objFilter = env.Program('seomFilter', ['src/main.c'] + src['common'])
env.Install('/usr/bin', objFilter)

objExample = env.Program('example', 'example.c')

for file in os.listdir('include/seom'):
	path = os.path.join('include/seom', file)
	if os.path.isfile(path):
		env.Install('/usr/include/seom', path)

env.Default([ objLibrary, objServer, objPlayer, objFilter, objExample ])
env.Alias('install', '/usr')
