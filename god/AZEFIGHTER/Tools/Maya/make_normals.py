# Build the normal attacks that the knight was missing (standing light kick and the crouching attacks),
# modeled on Ryu's normals, for the Mixamo knight rig.
#
# Approach (same as make_shoryuken.py):
#   * Start from the base pose (Idle or CrouchIdle, frame 0) and author key poses directly.
#   * Hips and spine are offsets from the base pose. Striking limbs are placed with IK (where the hand /
#     foot goes and which way the elbow / knee points). Feet that stay on the floor are IK'd to 'BASE'
#     so they do not slide while the hips move. Then everything is baked to FK.
#   * Clip length = the move's total frames at 60fps, so the engine plays it at speed 1.0 and the frame
#     where the limb is fully extended matches the hitbox start frame in the parameter ini.
#
# Rig facts (probe_axes.py): +Z forward, +X is the character's left.
#   Hips/Spine rotateX + : bend forward      rotateY + : right shoulder comes forward
# Base poses (probe_pose.py), cm:
#   Idle       hips (8.2, 85.3, -6.6)  lead = left side
#   CrouchIdle hips (-0.3, 51.5, 0.7)  L foot rel (17, -40, 42)  R foot rel (-14, -36, -31)
#              L shoulder rel (15, 38, 26)  R shoulder rel (-19, 42, 18)
import os
import sys
import maya.cmds as cmds
import maya.mel as mel
import maya.api.OpenMaya as om

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from limb_twist import hold_bend_direction, hold_end_roll, hold_foot  # noqa: E402

cmds.loadPlugin('fbxmaya', quiet=True)
BASE_DIR = r'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'
OUT_DIR = os.environ.get('NORMALS_OUT', BASE_DIR)
ONLY = [n for n in os.environ.get('NORMALS_ONLY', '').split(',') if n]
NS = 'mixamorig:'
BASE = 'BASE'  # target that stays where the base pose has it (planted foot, resting hand)

# ---------------------------------------------------------------------------------------------
# Each move: base clip, clip length (= total frames), and key poses.
# Pose keys (all optional, missing = base pose):
#   hips_dy        : hips height offset (cm)
#   hips_dz        : hips forward offset (cm). Lets the body follow a limb instead of stretching the planted leg
#   hips_rot       : hips world rotation offset (x: lean forward +, y: right side forward +, z: tilt)
#   spine          : (bend forward +, right shoulder forward +) added to each of Spine / Spine1 / Spine2
#   head           : bend forward + added to Neck and Head
#   base_guard     : (move option) both arms start from the Idle guard instead of the base clip's arms
#   guard          : sides ('l' / 'r') whose arm takes the joint angles of the Idle guard. The arm keeps the
#                    fighting-stance shape relative to the chest however the body turns
#   fist           : sides whose wrist and fingers take the Idle clenched fist (for the punching hand)
#   r_hand/l_hand/r_foot/l_foot : fist / foot position relative to the hips, BASE (stays where the base
#     pose has it) or (BASE, offset) (moved by offset from there, for a foot that steps).
#     The elbow / knee keeps its base direction around the limb on every frame and the fist / foot does
#     not roll (limb_twist.py), so no limb twists on its way out and back. A foot that is BASE in every
#     pose is planted: it also keeps its world orientation so it does not turn on the floor.
#     A limb listed in any pose is IK'd for the whole clip; frames 0 and the last frame are BASE.
# ---------------------------------------------------------------------------------------------
# 踏み込み: CrouchHeavyPunch moves the character forward by physics while the fist rises
# (forwardSpeed 3.0 x 6 frames = 0.3 game units = 21 cm at the 0.014 model scale).
# The rear foot stays planted in the world, so in the clip it slides back by the same amount.
STEP_IN_CM = 21.0

MOVES = {
    # しゃがみ待機: the Mixamo crouch with both hands up in guard. The crouching attacks start and end
    # in this pose, so no arm has to swing up from the hanging idle during an attack
    'CrouchIdle': dict(base='CrouchIdle', end=1, base_guard=True, poses={}),

    # 立ち弱K: a quick snap kick with the lead leg to the shin, body leaning back a little
    'LightKick': dict(base='Idle', end=14, poses={
        2: dict(hips_dy=-2.0, hips_rot=(-4.0, 8.0, 0.0),
                l_foot=(8.0, -45.0, 32.0), r_foot=BASE),
        4: dict(hips_dy=-3.0, hips_rot=(-10.0, 14.0, 0.0), spine=(-2.0, 0.0),
                l_foot=(6.0, -58.0, 76.0), r_foot=BASE),
        6: dict(hips_dy=-3.0, hips_rot=(-10.0, 14.0, 0.0), spine=(-2.0, 0.0),
                l_foot=(6.0, -58.0, 74.0), r_foot=BASE),
        9: dict(hips_dy=-2.0, hips_rot=(-4.0, 8.0, 0.0),
                l_foot=(8.0, -45.0, 32.0), r_foot=BASE),
    }),
    # しゃがみ弱P: short jab with the lead hand from the crouch
    'CrouchLightPunch': dict(base='CrouchIdle', end=12, base_guard=True, poses={
        2: dict(hips_rot=(4.0, 8.0, 0.0),
                l_hand=(10.0, 38.0, 72.0), l_foot=BASE, r_foot=BASE),
        5: dict(hips_rot=(4.0, 8.0, 0.0),
                l_hand=(10.0, 38.0, 70.0), l_foot=BASE, r_foot=BASE),
        8: dict(hips_rot=(2.0, 4.0, 0.0),
                l_hand=(22.0, 34.0, 46.0), l_foot=BASE, r_foot=BASE),
    }),
    # しゃがみ中P: straight punch with the rear hand, the right side turns forward
    'CrouchMediumPunch': dict(base='CrouchIdle', end=18, base_guard=True, poses={
        2: dict(hips_rot=(0.0, -8.0, 0.0),
                r_hand=(-18.0, 24.0, 26.0), l_foot=BASE, r_foot=BASE),
        5: dict(hips_rot=(6.0, 28.0, 0.0), spine=(2.0, 10.0),
                r_hand=(-4.0, 42.0, 84.0), l_foot=BASE, r_foot=BASE),
        8: dict(hips_rot=(6.0, 28.0, 0.0), spine=(2.0, 10.0),
                r_hand=(-4.0, 42.0, 82.0), l_foot=BASE, r_foot=BASE),
        12: dict(hips_rot=(2.0, 10.0, 0.0), spine=(0.0, 4.0),
                 r_hand=(-16.0, 26.0, 34.0), l_foot=BASE, r_foot=BASE),
    }),
    # しゃがみ強P: stepping crouching uppercut straight up, the anti-air normal. The lead foot steps in
    # while the hips rise (the rear knee leaves the floor), the back straightens and the rear fist drives
    # straight up past the face
    'CrouchHeavyPunch': dict(base='CrouchIdle', end=32, base_guard=True, poses={
        3: dict(hips_dy=4.0, hips_rot=(6.0, -12.0, 0.0), spine=(2.0, -6.0), head=-4.0,
                r_hand=(-14.0, 12.0, 28.0),
                l_foot=(BASE, (0.0, 8.0, 0.0)),
                r_foot=(BASE, (0.0, 0.0, -STEP_IN_CM * 0.5))),
        5: dict(hips_dy=10.0, hips_rot=(2.0, 4.0, 0.0), spine=(-2.0, 4.0), head=-8.0,
                r_hand=(-10.0, 58.0, 34.0),
                l_foot=(BASE, (0.0, 2.0, 0.0)),
                r_foot=(BASE, (0.0, 0.0, -STEP_IN_CM * 0.85))),
        6: dict(hips_dy=18.0, hips_rot=(-2.0, 10.0, 0.0), spine=(-3.0, 5.0), head=-12.0,
                r_hand=(-10.0, 78.0, 36.0), l_foot=BASE,
                r_foot=(BASE, (0.0, 0.0, -STEP_IN_CM))),
        12: dict(hips_dy=18.0, hips_rot=(-2.0, 9.0, 0.0), spine=(-3.0, 5.0), head=-11.0,
                 r_hand=(-10.0, 77.0, 34.0), l_foot=BASE,
                 r_foot=(BASE, (0.0, 0.0, -STEP_IN_CM))),
        20: dict(hips_dy=8.0, hips_rot=(3.0, 4.0, 0.0), spine=(0.0, 2.0), head=-4.0,
                 r_hand=(-16.0, 26.0, 34.0), l_foot=BASE,
                 r_foot=(BASE, (0.0, 0.0, -STEP_IN_CM))),
        26: dict(hips_dy=3.0,
                 r_hand=(-16.0, 26.0, 34.0), l_foot=BASE,
                 r_foot=BASE),
    }),
    # しゃがみ弱K: short low poke with the lead foot along the floor
    'CrouchLightKick': dict(base='CrouchIdle', end=13, base_guard=True, poses={
        2: dict(hips_rot=(-2.0, 6.0, 0.0),
                l_foot=(16.0, -36.0, 62.0), r_foot=BASE),
        4: dict(hips_rot=(-4.0, 8.0, 0.0),
                l_foot=(14.0, -40.0, 84.0), r_foot=BASE),
        6: dict(hips_rot=(-4.0, 8.0, 0.0),
                l_foot=(14.0, -40.0, 82.0), r_foot=BASE),
        9: dict(hips_rot=(-2.0, 4.0, 0.0),
                l_foot=(16.0, -38.0, 56.0), r_foot=BASE),
    }),
    # しゃがみ中K: the long low poke. Hips drop, the body leans back and the lead leg reaches far
    'CrouchMediumKick': dict(base='CrouchIdle', end=24, base_guard=True, poses={
        3: dict(hips_dy=-3.0, hips_rot=(-4.0, 8.0, 0.0),
                l_foot=(16.0, -38.0, 64.0), r_foot=BASE),
        7: dict(hips_dy=-8.0, hips_dz=14.0, hips_rot=(-12.0, 16.0, 0.0), spine=(-6.0, 0.0),
                l_foot=(12.0, -36.0, 100.0), r_foot=BASE),
        10: dict(hips_dy=-8.0, hips_dz=14.0, hips_rot=(-12.0, 16.0, 0.0), spine=(-6.0, 0.0),
                 l_foot=(12.0, -36.0, 98.0), r_foot=BASE),
        16: dict(hips_dy=-4.0, hips_dz=4.0, hips_rot=(-4.0, 6.0, 0.0),
                 l_foot=(16.0, -38.0, 58.0), r_foot=BASE),
    }),
    # しゃがみ強K: the sweep. The rear leg swings from behind, around the side, to the front
    # while the body pivots on the planted lead foot
    'CrouchHeavyKick': dict(base='CrouchIdle', end=34, base_guard=True, poses={
        3: dict(hips_dy=-6.0, hips_rot=(2.0, 18.0, 0.0), spine=(-2.0, 6.0), head=-6.0,
                r_foot=(-42.0, -36.0, -22.0), l_foot=BASE),
        6: dict(hips_dy=-8.0, hips_dz=10.0, hips_rot=(4.0, 32.0, 0.0), spine=(-3.0, 12.0), head=-8.0,
                r_foot=(-72.0, -36.0, 18.0), l_foot=BASE),
        8: dict(hips_dy=-8.0, hips_dz=14.0, hips_rot=(4.0, 42.0, 0.0), spine=(-3.0, 14.0), head=-8.0,
                r_foot=(-34.0, -36.0, 72.0), l_foot=BASE),
        11: dict(hips_dy=-8.0, hips_dz=14.0, hips_rot=(4.0, 44.0, 0.0), spine=(-3.0, 14.0), head=-8.0,
                 r_foot=(-24.0, -36.0, 78.0), l_foot=BASE),
        22: dict(hips_dy=-4.0, hips_dz=4.0, hips_rot=(2.0, 14.0, 0.0), spine=(-1.0, 4.0), head=-3.0,
                 r_foot=(-24.0, -38.0, -12.0), l_foot=BASE),
    }),
}

SPINE = ('Spine', 'Spine1', 'Spine2')
HEAD = ('Neck', 'Head')
LIMBS = {
    'r_hand': ('RightArm', 'RightForeArm', 'RightHand'),
    'l_hand': ('LeftArm', 'LeftForeArm', 'LeftHand'),
    'r_foot': ('RightUpLeg', 'RightLeg', 'RightFoot'),
    'l_foot': ('LeftUpLeg', 'LeftLeg', 'LeftFoot'),
}


def ws(joint):
    return cmds.xform(NS + joint, q=True, ws=True, t=True)


def base_pole(chain):
    """Point the elbow / knee the same way it points in the base pose."""
    start, mid, end = (ws(j) for j in chain)
    center = [(s + e) * 0.5 for s, e in zip(start, end)]
    return [m + (m - c) * 3.0 for m, c in zip(mid, center)]


GUARD_JOINTS = {
    'l': ('LeftShoulder', 'LeftArm', 'LeftForeArm', 'LeftHand'),
    'r': ('RightShoulder', 'RightArm', 'RightForeArm', 'RightHand'),
}


def capture_idle_guard():
    """Joint angles of the guarding arms, hands and clenched fingers in the Idle stance."""
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%sIdle.fbx"' % BASE_DIR)
    cmds.currentTime(0)
    angles = dict((NS + j, cmds.getAttr(NS + j + '.rotate')[0]) for js in GUARD_JOINTS.values() for j in js)
    for side, hand in (('l', 'LeftHand'), ('r', 'RightHand')):
        fingers = cmds.listRelatives(NS + hand, allDescendents=True, type='joint') or []
        FIST_JOINTS[side] = [NS + hand] + fingers
        for j in fingers:
            angles[j] = cmds.getAttr(j + '.rotate')[0]
    return angles


FIST_JOINTS = {}  # side -> hand and finger joints, filled by capture_idle_guard


def build(name, spec):
    end_frame = spec['end']
    poses = spec['poses']

    # ---- base pose ----
    cmds.file(new=True, force=True)
    cmds.currentUnit(time='ntscf')
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, spec['base']))
    cmds.currentTime(0)
    if spec.get('base_guard'):
        # both arms take the Idle guard (arm angles and clenched fingers) as the base pose
        for side in ('l', 'r'):
            for j in [NS + n for n in GUARD_JOINTS[side]] + FIST_JOINTS[side]:
                cmds.cutKey(j, clear=True)
                cmds.setAttr(j + '.rotate', *IDLE_GUARD[j])
    joints = cmds.ls(type='joint')
    base_rot = dict((j, cmds.getAttr(j + '.rotate')[0]) for j in joints)
    base_hips_t = cmds.getAttr(NS + 'Hips.translate')[0]
    base_hips_ro = cmds.xform(NS + 'Hips', q=True, ws=True, rotation=True)
    base_end = dict((k, ws(c[2])) for k, c in LIMBS.items())
    base_poles = dict((k, base_pole(c)) for k, c in LIMBS.items())

    cmds.delete(cmds.ls(type='mesh'))
    for t in cmds.ls(type='transform'):
        if cmds.objExists(t) and cmds.nodeType(t) == 'transform' and not cmds.listRelatives(t, allDescendents=True):
            cmds.delete(t)
    cmds.cutKey(joints, clear=True)
    cmds.playbackOptions(min=0, max=end_frame, animationStartTime=0, animationEndTime=end_frame)

    def reset():
        for j, r in base_rot.items():
            cmds.setAttr(j + '.rotate', *r)
        cmds.setAttr(NS + 'Hips.translate', *base_hips_t)

    def key_body(frame):
        cmds.setKeyframe(joints, attribute=['rotateX', 'rotateY', 'rotateZ'], time=frame, inTangentType='auto', outTangentType='auto')
        cmds.setKeyframe(NS + 'Hips', attribute=['translateX', 'translateY', 'translateZ'], time=frame, inTangentType='auto', outTangentType='auto')

    # ---- body ----
    reset(); key_body(0)
    for frame in sorted(poses):
        p = poses[frame]
        reset()
        cmds.setAttr(NS + 'Hips.translateY', base_hips_t[1] + p.get('hips_dy', 0.0))
        cmds.setAttr(NS + 'Hips.translateZ', base_hips_t[2] + p.get('hips_dz', 0.0))
        drx, dry, drz = p.get('hips_rot', (0.0, 0.0, 0.0))
        cmds.xform(NS + 'Hips', ws=True, rotation=(base_hips_ro[0] + drx, base_hips_ro[1] + dry, base_hips_ro[2] + drz))
        bend, twist = p.get('spine', (0.0, 0.0))
        for s in SPINE:
            rx, ry, rz = base_rot[NS + s]
            cmds.setAttr(NS + s + '.rotate', rx + bend, ry + twist, rz)
        for h in HEAD:
            rx, ry, rz = base_rot[NS + h]
            cmds.setAttr(NS + h + '.rotate', rx + p.get('head', 0.0), ry, rz)
        for side in p.get('guard', ()):
            for j in [NS + n for n in GUARD_JOINTS[side]] + FIST_JOINTS[side]:
                cmds.setAttr(j + '.rotate', *IDLE_GUARD[j])
        # the punching arm is placed by IK; only its wrist and fingers take the Idle clenched fist
        for side in p.get('fist', ()):
            for j in FIST_JOINTS[side]:
                cmds.setAttr(j + '.rotate', *IDLE_GUARD[j])
        key_body(frame)
    reset(); key_body(end_frame)
    cmds.filterCurve(cmds.listConnections(joints, type='animCurve') or [], filter='euler')

    # ---- limbs ----
    # Build, bake and remove one IK chain at a time. With several IK handles alive in one bake,
    # Maya sometimes writes the FK value instead of the IK result for the later chains
    # (a planted foot then slides with the hips), so each chain is solved on its own.
    # The chain keeps no FK keys while it is solved, and the last frame aims at the base pose.
    used = [k for k in LIMBS if any(k in p for p in poses.values())]
    frames = [0] + sorted(poses) + [end_frame]
    for key in used:
        start, mid, end = (NS + n for n in LIMBS[key])
        cmds.cutKey([start, mid], attribute=['rotateX', 'rotateY', 'rotateZ'], clear=True)
        # the solver bends the chain the way it is bent when the handle is made,
        # so start from the bent base pose, not from whatever the last bake left
        for j in (start, mid):
            cmds.setAttr(j + '.rotate', *base_rot[j])
        cmds.joint(start, edit=True, setPreferredAngles=True, children=True)

        # a fully straight limb has no bend plane and the solver twists it around itself, so every
        # target stays inside 93% of the limb's length and the elbow / knee always keeps a little bend
        reach = 0.93 * sum((om.MVector(*cmds.xform(a, q=True, ws=True, t=True)) - om.MVector(*cmds.xform(b, q=True, ws=True, t=True))).length()
                           for a, b in ((start, mid), (mid, end)))
        handle = cmds.ikHandle(startJoint=start, endEffector=end, solver='ikRPsolver', name=key + '_ik')[0]
        pole = cmds.spaceLocator(name=key + '_pole')[0]
        cmds.poleVectorConstraint(pole, handle)
        for frame in frames:
            spec_limb = poses.get(frame, {}).get(key, BASE)
            if spec_limb == BASE:
                target = base_end[key]
            elif spec_limb[0] == BASE:
                bx, by, bz = base_end[key]
                ox, oy, oz = spec_limb[1]
                target = (bx + ox, by + oy, bz + oz)
            else:
                hx, hy, hz = cmds.getAttr(NS + 'Hips.translate', time=frame)[0]
                tx, ty, tz = spec_limb
                target = (hx + tx, hy + ty, hz + tz)
            cmds.currentTime(frame)
            root = om.MVector(*cmds.xform(start, q=True, ws=True, t=True))
            to_target = om.MVector(*target) - root
            if to_target.length() > reach:
                p = root + to_target.normal() * reach
                target = (p.x, p.y, p.z)
            # the pole only has to give the base bend here; hold_bend_direction re-keys it below
            cmds.setAttr(handle + '.translate', *target)
            cmds.setAttr(pole + '.translate', *base_poles[key])
            cmds.setKeyframe(handle, attribute=['translateX', 'translateY', 'translateZ'], time=frame)
            cmds.setKeyframe(pole, attribute=['translateX', 'translateY', 'translateZ'], time=frame)

        hold_bend_direction(handle, pole, start, mid, end_frame)
        cmds.bakeResults([start, mid], time=(0, end_frame), sampleBy=1, simulation=True, preserveOutsideKeys=False,
                         attribute=['rotateX', 'rotateY', 'rotateZ'])
        cmds.delete(handle, pole)

    cmds.bakeResults(joints, time=(0, end_frame), sampleBy=1, simulation=True, preserveOutsideKeys=False,
                     attribute=['rotateX', 'rotateY', 'rotateZ', 'translateX', 'translateY', 'translateZ'])
    for key in used:
        start, mid, end = (NS + n for n in LIMBS[key])
        if key.endswith('_foot'):
            planted = all(p.get(key, BASE) == BASE or p[key][0] == BASE for p in poses.values())
            hold_foot(end, end_frame, planted)
        else:
            hold_end_roll(start, mid, end, end_frame)
    cmds.filterCurve(cmds.listConnections(joints, type='animCurve') or [], filter='euler')

    # ---- export ----
    out = os.path.join(OUT_DIR, name + '.fbx').replace('\\', '/')
    cmds.select(NS + 'Hips', hierarchy=True)
    mel.eval('FBXResetExport')
    mel.eval('FBXExportBakeComplexAnimation -v true')
    mel.eval('FBXExportBakeComplexStart -v 0')
    mel.eval('FBXExportBakeComplexEnd -v %d' % end_frame)
    mel.eval('FBXExportBakeComplexStep -v 1')
    mel.eval('FBXExportSkins -v false')
    mel.eval('FBXExportShapes -v false')
    mel.eval('FBXExportConstraints -v false')
    mel.eval('FBXExportInputConnections -v false')
    mel.eval('FBXExportUpAxis y')
    mel.eval('FBXExport -f "%s" -s' % out)
    print('RESULT exported', out, 'keys:', cmds.keyframe(NS + 'Hips.rotateX', q=True, keyframeCount=True))


IDLE_GUARD = capture_idle_guard()
for move_name, move_spec in MOVES.items():
    if ONLY and move_name not in ONLY:
        continue
    build(move_name, move_spec)
