//
// 264-toy
//

#include <iostream>

#include "pub.hpp"

using namespace std;

int __generic_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
	__generic_main(argc, argv);

	return 0;
}

int __generic_main(int argc, char *argv[])
{
	RtmpStream rtmp;

	if (argc != 4) {
		cout << "arguments not match! arg0="<< argv[0] << endl;
		exit(1);
	}

	char *pFileType = argv[1];
	if (rtmp.Connect(argv[2]) == true) {
		if (strcmp(pFileType, "h264") == 0) {
			rtmp.SendH264File(argv[3]);
		} else if (strcmp(pFileType, "aac") == 0) {
			rtmp.SendAacFile(argv[3]);
		}
	} else {
		cout << "Not connected !" << endl;
	}
	rtmp.Close();

	return 0;
}
