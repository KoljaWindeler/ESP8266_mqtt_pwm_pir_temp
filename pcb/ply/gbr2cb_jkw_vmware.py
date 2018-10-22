# python
#
# Tiny script to import a number of Gerber pcb files + drill into the CamBam
# jk 2015
# fwd_fx@spacenet.ru

from CamBam import *
from CamBam.CAD import *
from CamBam.CAM import *
from CamBam.Geom import *

import clr
clr.AddReference("System.Windows.Forms")
from System.Windows.Forms import OpenFileDialog
from System.Windows.Forms import DialogResult
from System.Drawing import Color
from System.Collections.Generic import List

SCALE = 25.4
DRILL_SCALE = 25.4
FLIP = 'x'

# select a file with a given type/extension via dialog
def ask_for_file(typ, exts):
	dlg = OpenFileDialog()
	dlg.Filter = typ + ' (' + ', '.join(['*.' + e for e in exts]) + ')|' + ';'.join(['*.' + e for e in exts])
	dlg.Multiselect = False
	ret = dlg.ShowDialog()
	return dlg.FileName if ret == DialogResult.OK else None

# get global bounding box of specified cad object
def get_global_extremas(cad):
	extremas = []
	for layer in cad.Layers:
		for e in layer.Entities:
			p0 = Point3F()
			p1 = Point3F()
			p0, p1 = e.GetExtrema(p0, p1, False)
			extremas += [(p0.X, p0.Y, p0.Z)]
			extremas += [(p1.X, p1.Y, p1.Z)]
	if not extremas:
		return None
	x0 = min([p[0] for p in extremas])
	x1 = max([p[0] for p in extremas])
	y0 = min([p[1] for p in extremas])
	y1 = max([p[1] for p in extremas])
	z0 = min([p[2] for p in extremas])
	z1 = max([p[2] for p in extremas])
	return (x0, y0, z0), (x1, y1, z1)

# scale cad object 
def scale_cad(cad, scale):
	matrix = Matrix4x4F(scale, 0, 0, 0, 0, scale, 0, 0, 0,  0, 1, 0,0, 0, 0, 0)
	for layer in cad.Layers:
		for e in layer.Entities:
			e.ApplyTransformation(matrix)

# move to origin ( all components seam to be moved by scale*0.01 )
def move_cad(cad, x,y):
	matrix = Matrix4x4F(1, 0, 0, 0, 0, 1, 0, 0, 0,  0, 1, 0, x, y, 0, 0)
	for layer in cad.Layers:
		for e in layer.Entities:
			e.ApplyTransformation(matrix)

def add_flip_points(cad,height):
	flippointlist = PointList()
	p0 = Point3F(0,(height/2 + SCALE/8),0)
	p1 = Point3F(0,(-height/2 - SCALE/8),0)
	flippointlist.Add(p0)
	flippointlist.Add(p1)
	cad.Add(flippointlist )

# flip cad object around the center of specified bounding box
def flip_cad(cad, bbox, flip):
	if flip == 'x':
		xscale = -1
		yscale = 1
		xoffset = (bbox[1][0] + bbox[0][0])
		yoffset = 0
	elif flip == 'y':
		xscale = 1
		yscale = -1
		xoffset = 0
		yoffset = (bbox[1][1] + bbox[0][1])
	else:
		assert 0, 'no flip specified'
	matrix = Matrix4x4F(xscale,           0, 0, 0,
                        0,        yscale, 0, 0,
                        0,                0, 1, 0,
                        xoffset, yoffset, 0, 0)
	for layer in cad.Layers:
		for e in layer.Entities:
			e.ApplyTransformation(matrix)

# merge old cad object into the new (existing) cad object
# old cam layers and parts are flattened
# resulting layer will get a specified name and color, if defined

def merge(ndoc, odoc, opts={}):
	if opts.get('flip'):
		bbox = get_global_extremas(ndoc)
		flip_cad(odoc, bbox, opts['flip'])
	# create a single layer
	nlayer = Layer()
	if 'layer_name' in opts:
		nlayer.Name = opts['layer_name']
	if 'layer_color' in opts:
		nlayer.Color = Color.FromArgb(*opts['layer_color'])
	ndoc.Layers.Add(nlayer)

	idmap = {}
	# copy cad data
	for layer in odoc.Layers:
		for e in layer.Entities:
			ne = e.Clone()
			nlayer.Entities.Add(ne)
			idmap[e.ID] = ne.ID
	# create a single part
	npart = CAMPart()
	if 'part_name' in opts:
		npart.Name = opts['part_name']
	ndoc.Parts.Add(npart)
	# copy cam data
	for part in odoc.Parts:
		for mop in part.MachineOps:
			m = mop.Clone()
			# remap
			mop.PrimitiveIds = List[int]([idmap[pid] for pid in mop.PrimitiveIds]).ToArray()
			npart.MachineOps.Add(mop)

# read file and create a new cad object
def import_file(file, reader):
	cad = CADFile()
	reader.ReadFile(file)
	reader.AddToDocument(cad)
	return cad

def main():
	topfile = ask_for_file('Top Gerber', ['gtl'])
	botfile = ask_for_file('Bot Gerber', ['gbl'])
	edgefile = ask_for_file('Edge Gerber', ['cut'])
	drillfile_top = ask_for_file('Drill top', ['drt'])
	drillfile_bot = ask_for_file('Drill bottom', ['drb'])

	if edgefile:
		print 'importing edge file ... '
		cad = import_file(edgefile, GerberFile())
		scale_cad(cad, SCALE)
		m1 = get_global_extremas(cad)
		m1_x_center=(m1[1][0]-m1[0][0])/2+m1[0][0]
		m1_y_center=(m1[1][1]-m1[0][1])/2+m1[0][1]
		print(m1_x_center)
		print(m1_y_center)
		move_cad(cad,-m1_x_center,-m1_y_center)
		merge(doc, cad, opts={'layer_name':'edge', 'part_name':'edge', 'layer_color':(200, 200, 0)})

		if topfile:
			print 'importing top file ... '
			cad = import_file(topfile, GerberFile())
			scale_cad(cad,SCALE)
			move_cad(cad,-m1_x_center,-m1_y_center)
			merge(doc, cad, opts={'layer_name':'top', 'part_name':'top', 'layer_color':(200, 0, 0)})
	
		if drillfile_top:
			print 'importing top drill file ... '
			cad = import_file(drillfile_top, CamBamUI.FindFileHandler('drl'))
			print 'scale top drill file ... '
			scale_cad(cad,DRILL_SCALE)
			print 'move top drill file ... '
			move_cad(cad,-m1_x_center,-m1_y_center)
			drill_top = get_global_extremas(cad)
			drill_top_x_center=(drill_top[1][0]-drill_top[0][0])/2+drill_top[0][0]
			drill_top_y_center=(drill_top[1][1]-drill_top[0][1])/2+drill_top[0][1]
			add_flip_points(cad,m1[1][1]-m1[0][1])
			merge(doc, cad, opts={'layer_name':'drill_top', 'part_name':'drill_top', 'style':'JKW_PCB drill'})

		if drillfile_bot:
			print 'importing bottom drill file ... '
			cad = import_file(drillfile_bot, CamBamUI.FindFileHandler('drl'))
			print 'scale bot drill file ... '
			scale_cad(cad,DRILL_SCALE)
			m_bot = get_global_extremas(cad)
			flip_cad(cad, m_bot, FLIP)
			print 'move bot drill file ... '
			move_cad(cad,-m1_x_center,-m1_y_center)			
			# so, we've moved the bot drill by the same amount as we've moved the top drill
			# the new center must be *(-1/0)  of the old center.. lets check
			drill_bot = get_global_extremas(cad)
			drill_bot_x_center=(drill_bot[1][0]-drill_bot[0][0])/2+drill_bot[0][0]
			drill_bot_y_center=(drill_bot[1][1]-drill_bot[0][1])/2+drill_bot[0][1]
			
			print 'old center '+str(drill_top_x_center)+' vs new '+str(drill_bot_x_center)
			if(drill_bot_x_center==-1*drill_top_x_center):
				print 'no missalignment'
				miss = 0
			else:
				print 'correcting missalignment'
				miss = -drill_top_x_center-drill_bot_x_center
				move_cad(cad,miss,0)
			merge(doc, cad, opts={'layer_name':'drill_bot', 'part_name':'drill_bot', 'style':'JKW_PCB drill'})

			if botfile:
				print 'importing bottom file ... '
				cad = import_file(botfile, GerberFile())
				scale_cad(cad,SCALE)
				print 'flipping bottom file ... '
				flip_cad(cad, m_bot, FLIP)
				move_cad(cad,-m1_x_center+miss,-m1_y_center)
				merge(doc, cad, opts={'layer_name':'bot', 'part_name':'bot', 'layer_color':(0, 100, 200)})


	print 'Done'

main()