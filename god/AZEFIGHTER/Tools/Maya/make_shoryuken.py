# Build a Shoryuken (rising uppercut, modeled on Ryu's) for the Mixamo knight rig.
#
# Approach:
#   * Every key pose is authored directly instead of borrowing another clip, because the move is
#     asymmetric: rear knee driven up, front leg trailing down-back, rear fist driving forward-up,
#     so the body forms one diagonal line leaning into the opponent.
#   * Hips and spine are posed with world / local rotations. Arms and legs are placed with IK
#     (where the hand / foot goes and which way the elbow / knee points), then baked to FK.
#   * The game lifts the character with physics, so the clip stays roughly in place.
#     Takeoff (frame 2) and landing (frame 18) must match Shoryuken::TAKEOFF_RATIO / LANDING_RATIO.
#   * Every frame is baked, because the engine samples keys by index (frame % keyCount).
#
# Rig facts (measured with probe_axes.py): +Z forward, +X is the character's left.
#   Hips/Spine rotateX + : bend forward      rotateY + : right shoulder comes forward
#   Hips world rotateY 0 : chest square to the front (the idle stance is about -53)
import os
import sys
import maya.cmds as cmds
import maya.mel as mel

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from limb_twist import hold_bend_direction, hold_end_roll, smooth_foot  # noqa: E402

cmds.loadPlugin('fbxmaya', quiet=True)
BASE = r'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'
OUT_FBX = os.environ.get('SHORYU_OUT', r'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/Shoryuken.fbx')
NS = 'mixamorig:'
END_FRAME = 24
GROUND = None  # marker: foot target Y is absolute (planted on the floor) instead of relative to the hips

# ---------------------------------------------------------------------------------------------
# Key poses. Positions are cm relative to the hips (x: +left, y: up, z: +forward).
#   hips_y       : hips height
#   hips_rot     : hips world rotation (x: lean forward +, y: turn, z: side tilt)
#   spine        : (bend forward +, right shoulder forward +) added to each of Spine / Spine1 / Spine2
#   head         : bend forward + added to Neck and Head (negative looks up)
#   r_hand/l_hand: (target, elbow pole)
#   r_foot/l_foot: (target, knee pole). A target y of GROUND plants the foot at ankle height.
# ---------------------------------------------------------------------------------------------
# the idle stance feet, so the landing and the return to the stance do not slide
L_FOOT_PLANT = (16.0, GROUND, 28.6)
R_FOOT_PLANT = (-18.0, GROUND, -22.2)
ANKLE_HEIGHT = 11.6
# Elbow poles always point outward. Moving a pole from behind the arm to in front of it flips the
# elbow plane and twists the whole arm within a frame or two (measured with check_arm_roll.py).

POSES = {
    # sink in the stance and load the rear fist low
    0: dict(hips_y=74.0, hips_rot=(14.0, -38.0, 0.0), spine=(4.0, -4.0), head=0.0,
            r_hand=((-12.0, 6.0, 20.0), (-55.0, 0.0, 12.0)),
            l_hand=((8.0, 38.0, 28.0), (55.0, 0.0, 5.0)),
            l_foot=(L_FOOT_PLANT, (16.0, 40.0, 80.0)),
            r_foot=(R_FOOT_PLANT, (-16.0, 40.0, 60.0))),
    # deepest dip; the hips already start turning toward the opponent so the takeoff is not a snap
    1: dict(hips_y=64.0, hips_rot=(20.0, -30.0, 0.0), spine=(7.0, -6.0), head=4.0,
            r_hand=((-6.0, -4.0, 24.0), (-55.0, 0.0, 12.0)),
            l_hand=((12.0, 34.0, 24.0), (55.0, 0.0, 5.0)),
            l_foot=(L_FOOT_PLANT, (16.0, 40.0, 80.0)),
            r_foot=(R_FOOT_PLANT, (-16.0, 40.0, 60.0))),
    # takeoff: lunge into the opponent, chest turns square, fist rises past the chest
    2: dict(hips_y=88.0, hips_rot=(12.0, -14.0, 0.0), spine=(4.0, 8.0), head=0.0,
            r_hand=((-2.0, 40.0, 28.0), (-55.0, 10.0, 20.0)),
            l_hand=((20.0, 26.0, 6.0), (55.0, 0.0, 5.0)),
            l_foot=((12.0, -70.0, -2.0), (12.0, -35.0, 80.0)),
            r_foot=((-10.0, -50.0, 18.0), (-10.0, -20.0, 80.0))),
    # fist passing in front of the face on a forward-up path
    3: dict(hips_y=93.0, hips_rot=(12.0, 0.0, 0.0), spine=(4.0, 10.0), head=-2.0,
            r_hand=((-4.0, 70.0, 34.0), (-52.0, 20.0, 28.0)),
            l_hand=((20.0, 16.0, 0.0), (55.0, 0.0, 5.0)),
            l_foot=((10.0, -76.0, -8.0), (10.0, -40.0, 80.0)),
            r_foot=((-9.0, -44.0, 16.0), (-9.0, -10.0, 85.0))),
    # full extension: one diagonal line from the trailing foot through the body to the fist.
    # The fist drives forward-up, not straight up, and the body leans into the opponent
    4: dict(hips_y=96.0, hips_rot=(14.0, 10.0, 0.0), spine=(3.0, 10.0), head=-6.0,
            r_hand=((-6.0, 96.0, 44.0), (-50.0, 40.0, 35.0)),
            l_hand=((20.0, 10.0, -6.0), (55.0, 0.0, 5.0)),
            l_foot=((10.0, -76.0, -20.0), (10.0, -40.0, 80.0)),
            r_foot=((-8.0, -40.0, 14.0), (-8.0, 0.0, 90.0))),
    # hold the pose at the top
    10: dict(hips_y=95.0, hips_rot=(10.0, 10.0, 0.0), spine=(2.0, 8.0), head=-4.0,
             r_hand=((-6.0, 96.0, 36.0), (-50.0, 40.0, 35.0)),
             l_hand=((20.0, 12.0, -4.0), (55.0, 0.0, 5.0)),
             l_foot=((10.0, -76.0, -16.0), (10.0, -40.0, 80.0)),
             r_foot=((-8.0, -42.0, 12.0), (-8.0, 0.0, 90.0))),
    # start falling almost straight down, body straightens, the knee comes down first
    14: dict(hips_y=92.0, hips_rot=(2.0, 4.0, 0.0), spine=(0.0, 4.0), head=-2.0,
             r_hand=((-10.0, 84.0, 16.0), (-50.0, 30.0, 25.0)),
             l_hand=((20.0, 26.0, 10.0), (55.0, 0.0, 5.0)),
             l_foot=((10.0, -74.0, 8.0), (10.0, -40.0, 80.0)),
             r_foot=((-12.0, -60.0, 16.0), (-12.0, -20.0, 80.0))),
    # reaching for the floor, arms coming back to guard
    17: dict(hips_y=88.0, hips_rot=(8.0, -26.0, 0.0), spine=(2.0, 0.0), head=0.0,
             r_hand=((-12.0, 40.0, 22.0), (-55.0, 5.0, 12.0)),
             l_hand=((12.0, 34.0, 22.0), (55.0, 0.0, 5.0)),
             l_foot=((14.0, -72.0, 18.0), (14.0, -35.0, 80.0)),
             r_foot=((-14.0, -70.0, -10.0), (-14.0, -35.0, 70.0))),
    # landing impact
    18: dict(hips_y=66.0, hips_rot=(18.0, -34.0, 0.0), spine=(6.0, -2.0), head=2.0,
             r_hand=((-18.0, 34.0, 18.0), (-55.0, 0.0, 12.0)),
             l_hand=((10.0, 34.0, 26.0), (55.0, 0.0, 5.0)),
             l_foot=(L_FOOT_PLANT, (16.0, 40.0, 80.0)),
             r_foot=(R_FOOT_PLANT, (-16.0, 40.0, 60.0))),
}
# frame 24 is the idle stance: the body returns to the idle FK pose and the limbs aim at the idle hands / feet

SPINE = ('Spine', 'Spine1', 'Spine2')
HEAD = ('Neck', 'Head')
IK_CHAINS = {
    'r_hand': ('RightArm', 'RightForeArm', 'RightHand'),
    'l_hand': ('LeftArm', 'LeftForeArm', 'LeftHand'),
    'r_foot': ('RightUpLeg', 'RightLeg', 'RightFoot'),
    'l_foot': ('LeftUpLeg', 'LeftLeg', 'LeftFoot'),
}

# ---- 1. rig from Idle ------------------------------------------------------------------------------
cmds.file(new=True, force=True)
cmds.currentUnit(time='ntscf')
mel.eval('FBXImport -f "%sIdle.fbx"' % BASE)
cmds.currentTime(0)
joints = cmds.ls(type='joint')
idle_rot = dict((j, cmds.getAttr(j + '.rotate')[0]) for j in joints)
idle_hips_t = cmds.getAttr(NS + 'Hips.translate')[0]

def ws(joint):
    return cmds.xform(NS + joint, q=True, ws=True, t=True)

def stance_pole(chain):
    start, mid, end = (ws(j) for j in chain)
    center = [(s + e) * 0.5 for s, e in zip(start, end)]
    return [m + (m - c) * 3.0 for m, c in zip(mid, center)]

idle_end = dict((k, ws(c[2])) for k, c in IK_CHAINS.items())
idle_pole = dict((k, stance_pole(c)) for k, c in IK_CHAINS.items())
cmds.delete(cmds.ls(type='mesh'))
for t in cmds.ls(type='transform'):
    if cmds.objExists(t) and cmds.nodeType(t) == 'transform' and not cmds.listRelatives(t, allDescendents=True):
        cmds.delete(t)
cmds.cutKey(joints, clear=True)
cmds.playbackOptions(min=0, max=END_FRAME, animationStartTime=0, animationEndTime=END_FRAME)

def key_all(frame):
    cmds.setKeyframe(joints, attribute=['rotateX', 'rotateY', 'rotateZ'], time=frame, inTangentType='auto', outTangentType='auto')
    cmds.setKeyframe(NS + 'Hips', attribute=['translateX', 'translateY', 'translateZ'], time=frame, inTangentType='auto', outTangentType='auto')

def reset_to_idle():
    for j, r in idle_rot.items():
        cmds.setAttr(j + '.rotate', *r)
    cmds.setAttr(NS + 'Hips.translate', *idle_hips_t)

# ---- 2. body (hips, spine, head) -----------------------------------------------------------------
for frame in sorted(POSES):
    p = POSES[frame]
    reset_to_idle()
    cmds.setAttr(NS + 'Hips.translate', idle_hips_t[0], p['hips_y'], idle_hips_t[2])
    cmds.xform(NS + 'Hips', ws=True, rotation=p['hips_rot'])
    bend, twist = p['spine']
    for s in SPINE:
        rx, ry, rz = idle_rot[NS + s]
        cmds.setAttr(NS + s + '.rotate', rx + bend, ry + twist, rz)
    for h in HEAD:
        rx, ry, rz = idle_rot[NS + h]
        cmds.setAttr(NS + h + '.rotate', rx + p['head'], ry, rz)
    key_all(frame)
reset_to_idle()
key_all(END_FRAME)
cmds.filterCurve(cmds.listConnections(joints, type='animCurve') or [], filter='euler')

# ---- 3. limbs with IK -----------------------------------------------------------------------------
# One chain at a time: with several IK handles alive in one bake, Maya sometimes writes the FK value
# instead of the IK result for the later chains. The last frame aims at the idle stance.
for key, chain in IK_CHAINS.items():
    start, mid, end = (NS + n for n in chain)
    cmds.cutKey([start, mid], attribute=['rotateX', 'rotateY', 'rotateZ'], clear=True)
    # the solver bends the chain the way it is bent when the handle is made
    for j in (start, mid):
        cmds.setAttr(j + '.rotate', *idle_rot[j])
    cmds.joint(start, edit=True, setPreferredAngles=True, children=True)

    handle = cmds.ikHandle(startJoint=start, endEffector=end, solver='ikRPsolver', name=key + '_ik')[0]
    pole = cmds.spaceLocator(name=key + '_pole')[0]
    cmds.poleVectorConstraint(pole, handle)
    for frame in sorted(POSES) + [END_FRAME]:
        if frame == END_FRAME:
            target, pole_pos = idle_end[key], idle_pole[key]
        else:
            (tx, ty, tz), (px, py, pz) = POSES[frame][key]
            hx, hy, hz = cmds.getAttr(NS + 'Hips.translate', time=frame)[0]
            target = (hx + tx, ANKLE_HEIGHT if ty is GROUND else hy + ty, hz + tz)
            pole_pos = (hx + px, hy + py, hz + pz)
        cmds.setAttr(handle + '.translate', *target)
        cmds.setAttr(pole + '.translate', *pole_pos)
        cmds.setKeyframe(handle, attribute=['translateX', 'translateY', 'translateZ'], time=frame)
        cmds.setKeyframe(pole, attribute=['translateX', 'translateY', 'translateZ'], time=frame)
    hold_bend_direction(handle, pole, start, mid, END_FRAME)

    cmds.bakeResults([start, mid], time=(0, END_FRAME), sampleBy=1, simulation=True, preserveOutsideKeys=False,
                     attribute=['rotateX', 'rotateY', 'rotateZ'])
    cmds.delete(handle, pole)

cmds.bakeResults(joints, time=(0, END_FRAME), sampleBy=1, simulation=True, preserveOutsideKeys=False,
                 attribute=['rotateX', 'rotateY', 'rotateZ', 'translateX', 'translateY', 'translateZ'])
for chain in IK_CHAINS.values():
    hold_end_roll(*(NS + n for n in chain), end_frame=END_FRAME)
smooth_foot(NS + 'RightFoot', END_FRAME)
smooth_foot(NS + 'LeftFoot', END_FRAME)
cmds.filterCurve(cmds.listConnections(joints, type='animCurve') or [], filter='euler')

# ---- 4. export -------------------------------------------------------------------------------------
cmds.select(NS + 'Hips', hierarchy=True)
mel.eval('FBXResetExport')
mel.eval('FBXExportBakeComplexAnimation -v true')
mel.eval('FBXExportBakeComplexStart -v 0')
mel.eval('FBXExportBakeComplexEnd -v %d' % END_FRAME)
mel.eval('FBXExportBakeComplexStep -v 1')
mel.eval('FBXExportSkins -v false')
mel.eval('FBXExportShapes -v false')
mel.eval('FBXExportConstraints -v false')
mel.eval('FBXExportInputConnections -v false')
mel.eval('FBXExportUpAxis y')
mel.eval('FBXExport -f "%s" -s' % OUT_FBX.replace('\\', '/'))
print('RESULT exported', OUT_FBX, 'keys per curve:', cmds.keyframe(NS + 'RightArm.rotateX', q=True, keyframeCount=True))
