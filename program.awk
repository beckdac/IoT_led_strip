BEGIN {
	printf("set timeout 5\n\n");
	printf("spawn telnet %s\n\n", host);
	printf("expect \"'^]'.\"\n");
	printf("send \"NOOP\\n\"\n");
	printf("expect \"ERROR\"\n");
	printf("send \"PROGRAM\\n\"\n");
	printf("expect \"OK\"\n");
	printf("send \"LENGTH %d\\n\"\n", program_length);
	printf("expect \"OK\"\n");
	step = 0;
}
{
	if ($1 == "STEP") {
		printf("send \"STEP %d %d %d %d %d\\n\"\n", step, $2, $3, $4, $5);
		printf("expect \"OK\"\n");
		step = step + 1;
	}
}
END {
	printf("send \"RUN\\n\"\n");
	printf("expect \"OK\"\n");
	printf("send \"DUMP\\n\"\n");
	printf("expect \"OK\"\n");
}
