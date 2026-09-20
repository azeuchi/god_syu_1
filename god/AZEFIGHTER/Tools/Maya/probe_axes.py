# Measure how each control axis moves the Mixamo knight, so key poses can be authored with the right signs.
import maya.cmds as cmds
import maya.mel as mel

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
cmds.file(new=True, force=True)
mel.eval('FBXImport -f "C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/Idle.fbx"')
cmds.currentTime(0)
joints = cmds.ls(type='joint')
base = dict((j, cmds.getAttr(j + '.rotate')[0]) for j in joints)
base_hips_t = cmds.getAttr(NS + 'Hips.translate')[0]
cmds.cutKey(joints, clear=True)

def reset():
    for j, r in base.items():
        cmds.setAttr(j + '.rotate', *r)
    cmds.setAttr(NS + 'Hips.translate', *base_hips_t)

def ws(name):
    return cmds.xform(NS + name, q=True, ws=True, t=True)

def report(label):
    head = ws('Head'); ls = ws('LeftArm'); rs = ws('RightArm'); lk = ws('LeftLeg'); rk = ws('RightLeg')
    print('RESULT %-22s head=(%.0f,%.0f,%.0f) Lsh=(%.0f,%.0f,%.0f) Rsh=(%.0f,%.0f,%.0f) Lknee=(%.0f,%.0f,%.0f) Rknee=(%.0f,%.0f,%.0f)' % (
        (label,) + tuple(head) + tuple(ls) + tuple(rs) + tuple(lk) + tuple(rk)))

reset(); report('idle')
print('RESULT hips world rot', [round(v, 1) for v in cmds.xform(NS + 'Hips', q=True, ws=True, ro=True)])

for joint in ('Hips', 'Spine1'):
    for axis in ('X', 'Y', 'Z'):
        reset()
        cmds.setAttr(NS + joint + '.rotate' + axis, base[NS + joint][' XYZ'.index(axis) - 1] + 25.0)
        report('%s rot%s +25' % (joint, axis))

# world-space hips rotation: square the body to the front
for ry in (0.0, -20.0):
    reset()
    cmds.xform(NS + 'Hips', ws=True, ro=(0.0, ry, 0.0))
    report('hips world ro y=%d' % ry)

for side in ('Left', 'Right'):
    j = NS + side + 'UpLeg'
    print('RESULT %sUpLeg orient' % side, [round(v, 1) for v in cmds.getAttr(j + '.jointOrient')[0]], 'rot', [round(v, 1) for v in base[j]])
