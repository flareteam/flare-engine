#! /usr/bin/python
import os
import datetime
import codecs   # proper UTF8 handling with files

keys = []
comments = []
now = datetime.datetime.now()
header = r'''# Copyright (C) 2011 Clint Bellanger
# This file is distributed under the same license as the FLARE package.
#
# FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.
msgid ""
msgstr ""
"Project-Id-Version: PACKAGE VERSION\n"
"Report-Msgid-Bugs-To: \n"
"POT-Creation-Date: {now}\n"
"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n"
"Last-Translator: FULL NAME <EMAIL@ADDRESS>\n"
"Language-Team: LANGUAGE <LL@li.org>\n"
"Language: \n"
"MIME-Version: 1.0\n"
"Content-Type: text/plain; charset=UTF-8\n"
"Content-Transfer-Encoding: 8bit\n"

'''

POT_STRING = u'''\
#: {comment}
msgid "{msgid}"
msgstr ""

'''

triggers = [
	'msg', 'him', 'her', 'you', 'name', 'title', 'tooltip',
	'power_desc', 'quest_text', 'description',
	'tab_title', 'currency_name', 'flavor', 'topic', 'option',
	'caption', 'text', 'name_short',

	# engine/resource_stats.txt
	'text_base', 'text_base_desc',
	'text_regen', 'text_regen_desc',
	'text_steal', 'text_steal_desc',
	'text_resist_steal', 'text_resist_steal_desc',
	'text_combat_heal', 'text_log_restore', 'text_log_low',
	'text_tooltip_heal', 'text_tooltip_cost',
]

plain_text = [
	'msg', 'him', 'her', 'you', 'name', 'title', 'tooltip',
	'quest_text', 'description', 'topic', 'flavor', 'caption', 'text'
]

allowed_credits_strings = []
credits_files = []

default_credits_settings = os.sys.path[0] + '/../../mods/default/languages/credits_settings.txt'

def parse_credits_settings(filename):
	if not os.path.exists(filename):
		print('Unable to load credits settings: ' + filename)
		if filename != default_credits_settings:
			parse_credits_settings(default_credits_settings)
		return

	if filename == default_credits_settings:
		print('Using default credits settings: ' + filename)

	infile = codecs.open(filename, encoding='UTF-8', mode='r')

	for line in infile:
		if line.startswith('s'):
			line = line.split('=')[1]
			allowed_credits_strings.append(line.rstrip())
		elif line.startswith('file'):
			line = line.split('=')[1]
			credits_files.append(line.rstrip())


# this extracts translatable strings from the flare data file
def extract(filename):
	if not os.path.exists(filename):
		return

	infile = codecs.open(filename, encoding='UTF-8', mode='r')

	is_credits_file = False
	for credits_file in credits_files:
		if filename.endswith(credits_file):
			is_credits_file = True
			break

	for i, line in enumerate(infile, start=1):
		for trigger in triggers:
			if line.startswith(trigger + '='):
				line = line.split('=')[1]
				line = line.strip('\n')
				values = line.split(',')
				# TODO checking length of values isn't reliable, check trigger too?
				if (trigger in plain_text):
				   stat = line.replace("\"", "\\\"");
				elif len(values) == 1:
				   # {key}={value}
				   stat, = values
				elif len(values) == 2:
				   # bonus={stat},{value}
				   stat, value = values
				elif len(values) == 3:
				   # bonus={set_level},{stat},{value}
				   set_level, stat, value = values
				elif len(values) == 5:
				   # option=base,head,portrait,name
				   stat = values[-1]

				test_key = stat.rstrip()
				if test_key == "":
					continue

				if is_credits_file and test_key not in allowed_credits_strings:
					continue

				comment = filename + ':' + str(i)
				comments.append(comment)
				keys.append(test_key)

# this removes duplicates from keys in a clean way (without screwing up the order)
def remove_duplicates():
	global comments
	global keys
	tmp = []
	tmp_c = []
	for node_c,node in zip(comments,keys):
		if node not in tmp:
			tmp_c.append(node_c)
			tmp.append(node)
	comments = tmp_c
	keys = tmp

# this writes the list of keys to a gettext .po file
def save(filename):
	outfile = codecs.open('data.pot', encoding='UTF-8', mode='w')
	outfile.write(header.format(now=now.strftime('%Y-%m-%d %H:%M+%z')))
	remove_duplicates()
	for line_c,line in zip(comments,keys):
		outfile.write(POT_STRING.format(comment=line_c, msgid=line))

# HERE'S THE MAIN EXECUTION
parse_credits_settings('credits_settings.txt')
extract('../items/items.txt')
extract('../items/types.txt')
extract('../items/sets.txt')
extract('../items/qualities.txt')
extract('../menus/powers.txt')
extract('../powers/effects.txt')
extract('../powers/powers.txt')
extract('../engine/elements.txt')
extract('../engine/loot.txt')
extract('../engine/classes.txt')
extract('../engine/hero_options.txt')
extract('../engine/titles.txt')
extract('../engine/equip_flags.txt')
extract('../engine/primary_stats.txt')
extract('../engine/damage_types.txt')
extract('../engine/resource_stats.txt')
extract('../soundfx/subtitles.txt')

for folder in ['enemies', 'maps', 'quests', 'npcs', 'cutscenes', 'books', 'items', 'powers', 'scripts']:
	target = os.path.join('..', folder)
	if os.path.isdir(target):
		for filename in sorted(os.listdir(target)):
				subfolder = os.path.join(target, filename)
				if os.path.isdir(subfolder):
					for root, dirs, files in os.walk(subfolder):
						for f in files:
							f = os.path.join(root, f)
							if os.path.isfile(f):
								extract(f)
				elif os.path.isfile(subfolder):
					extract(subfolder)

save('data.pot')
