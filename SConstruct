
import os
import SCons.Defaults
import SCons.Tool
import SCons.Util

arch = {
	'i386':('x86','elf32','-m32','lib32'),
	'i486':('x86','elf32','-m32','lib32'),
	'i586':('x86','elf32','-m32','lib32'),
	'i686':('x86','elf32','-m32','lib32'),
	'x86_64':('amd64','elf64','-m64','lib64'),
}[os.getenv('ARCH') or os.uname()[4]]

env = Environment(
	CC = 'gcc',
	CPPPATH = ['#include'],
	CCFLAGS = ['-std=c99', '-pipe', '-O3', arch[2]],
	LINKFLAGS = [arch[2]],
	LIBS = ['dl', 'pthread'],
	AS = 'yasm',
	ASFLAGS = '-f '+arch[1]
)

static_obj, shared_obj = SCons.Tool.createObjBuilders(env)

shared_obj.add_action('.asm', SCons.Defaults.ASAction)
shared_obj.add_emitter('.asm', SCons.Defaults.SharedObjectEmitter)

srcAssembler = [
	'src/asm/'+arch[0]+'/resample.asm',
	'src/asm/'+arch[0]+'/convert.asm',
	'src/asm/'+arch[0]+'/huffman.asm',
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
env.Install('/usr/'+arch[3], objLibrary)

objServer = env.Program('seomServer', srcServer)
env.Install('/usr/bin', objServer)

env = env.Copy()
env.Append(LIBS = ['GL', 'X11', 'seom'])
objPlayer = env.Program('seomPlayer', srcPlayer)
env.Depends(objPlayer, '/usr/'+arch[3])
env.Install('/usr/bin', objPlayer)

for file in os.listdir('include/seom'):
	if file[0] != '.':
		path = os.path.join('include/seom', file)
		env.Install('/usr/include/seom', path)

test = env.Program('test', 'test.c')

env.Default([ objLibrary, objServer, objPlayer ])
env.Alias('install', '/usr')
