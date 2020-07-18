#include <Arduino.h>

void NtpInit();
void NtpLoop();
//prototypes
String getFormattedDate();
//void getCurrentTime(struct tm *now);
String getCurrentTime();
String getFormattedDateTime();
struct tm* NtpCurrentTime();
