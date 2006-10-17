
import os

env = Environment(
	CC = 'gcc',
	CPPPATH = ['#include'],
	CCFLAGS = ['-std=c99', '-pipe', '-O3', '-W', '-Wall', os.getenv('CFLAGS')],
	LINKFLAGS = [os.getenv('LDFLAGS')],
	LIBS = ['dl', 'pthread'],
)

src = {
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

objLibrary = env.SharedLibrary('seom', src['lib'])
env.Install('/usr/lib', objLibrary)

env = env.Copy()
env.Append(LIBS = ['seom'], LIBPATH = ['.'])

objServer = env.Program('seomServer', src['server'])
env.Install('/usr/bin', objServer)

objFilter = env.Program('seomFilter', ['src/filter/main.c'])
env.Install('/usr/bin', objFilter)

env = env.Copy()
env.Append(LIBS = ['GL', 'X11'])

objPlayer = env.Program('seomPlayer', src['player'])
env.Install('/usr/bin', objPlayer)

objExample = env.Program('example', 'example.c')

for file in os.listdir('include/seom'):
	path = os.path.join('include/seom', file)
	if os.path.isfile(path):
		env.Install('/usr/include/seom', path)

env.Default([ objLibrary, objServer, objPlayer, objFilter, objExample ])
env.Alias('install', '/usr')
