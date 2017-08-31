
goal: IRCClient

IRCClient: IRCClient.cc
	g++ -g -o IRCClient IRCClient.cc `pkg-config --cflags --libs gtk+-2.0`

clean:
	rm -f hello panned entry radio timer TestIRCServer IRCClient IRCClientTimer

