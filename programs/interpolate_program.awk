{
	if ($1 == "STEP") {
		#print $0;
		r = $2;
		g = $3;
		b = $4;
		ms = $5;
		if (step > 0) {
			rdiv = (r - rl) / divisions;
			gdiv = (g - gl) / divisions;
			bdiv = (b - bl) / divisions;
			# printf("%d\t%d\t%d\n", rdiv, gdiv, bdiv);
			for (i = 0; i < divisions; ++i) {
				printf("STEP\t%d\t%d\t%d\t%d\n", rl + (i * rdiv), gl + (i * gdiv), bl + (i * bdiv), ms / divisions);
			}
		}
		rl = r;
		gl = g;
		bl = b;
		step = step + 1;
	}
}
END {
	# finish the final value
	printf("STEP\t%d\t%d\t%d\t%d\n", rl, gl, bl, ms / divisions);
}
