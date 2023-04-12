
all: inject_80211 analyze_80211

inject_80211: inject_80211.c
#	gcc  -Wall -Werror inject_80211.c -o inject_80211 -lpcap
	gcc  -Wall inject_80211.c -o inject_80211 -lpcap

analyze_80211: analyze_80211.c
# gcc  -Wall -Werror radiotap.c analyze_80211.c -o analyze_80211 -lpcap
	gcc  -Wall radiotap.c analyze_80211.c -o analyze_80211 -lpcap

clean:
	rm -f inject_80211 analyze_80211

