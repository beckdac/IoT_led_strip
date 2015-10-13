BEGIN {
	n = 10;
	dev = 20;
}
/^STEP/ {
		r = $2;
		g = $3;
		b = $4;
		t = $5;

		randomizeColor();
		print $0;
		randomizeColor();
}
function randMax(n) { return 1 + int(rand() * n) }
function randomizeColor() {
		for (i = 0; i < n; ++i) {
			rt = r + randMax(dev * 2) - (dev / 2);
			if (rt >= 255)
					rt = 255;
			if (rt < 0)
					rt = 0;
			gt = g + randMax(dev * 2) - (dev / 2);
			if (gt >= 255)
					gt = 255;
			if (gt < 0)
					gt = 0;
			bt = b + randMax(dev * 2) - (dev / 2);
			if (bt >= 255)
					bt = 255;
			if (bt < 0)
					bt = 0;
			tot = t / n;
			printf("STEP\t%d\t%d\t%d\t%d\n", rt, gt, bt, tot);
		}
}
