from math import sqrt, exp, pi

def create_gaussian(sigma, radius):
	weighted_sum = 0.0
	G = []
	for x in range(0, radius + 1):
		y = 1.0/sqrt(2.0 * pi * sigma * sigma) * exp(-float(x*x) / (2.0 * sigma * sigma))
		G.append(y)
		if x == 0:
			weighted_sum += y
		else:
			weighted_sum += 2.0 * y
	wG = []
	for x in reversed(G):
		wG.append(x / weighted_sum)
	for x in G[1:]:
		wG.append(x / weighted_sum)
	return wG

if __name__ == '__main__':
	wG = create_gaussian(2.5, 7)
	for x in wG: print "%gf, " % x,
	