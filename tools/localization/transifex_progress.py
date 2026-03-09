#!/usr/bin/env python

import sys

if __name__ == '__main__':
	if len(sys.argv) > 1:
		filename = sys.argv[1]
		print("## flare-engine Translation Status")
		print("| Language                      | Completed")
		print("|-------------------------------|----------------")
		with open(filename, 'r') as file:
			for line in file:
				# skip the first 3 lines
				if line.startswith('name,'):
					continue
				if line.startswith('English [source language],'):
					continue
				if line.startswith('all languages,'):
					continue

				# Turkish is currently at 0% on both the engine and game data
				# Will include it in the list if someone does an inital contribution.
				if line.startswith('Turkish'):
					continue

				# Strip the comma out of "Gaelic, Scottish"
				if line.startswith('"Gaelic, Scottish"'):
					line = line.replace('"Gaelic, Scottish"', 'Gaelic (Scottish)')

				split_line = line.split(',')
				language = (split_line[0] + ' (' + split_line[1].replace('_', '\\_') + ')').ljust(30)
				completion = round(float(split_line[7].rstrip('%')))

				print("| " + language + "| " + str(completion) + "%")
	else:
		print("Takes a report CSV from Transifex and outputs the Markdown that can be inserted in the README.")
		print("Usage: " + sys.argv[0] + " <filename>")
