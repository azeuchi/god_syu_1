# Measure how much each arm twists around itself from one frame to the next.
#   elbow swivel : where the elbow points around the shoulder-hand line
#   wrist roll   : how the hand turns around the forearm
# A natural punch changes these slowly; big jumps are what reads as "the arm spinning".
# Swinging the whole arm (a fist going from the chin to straight out) is taken out, only the turn
# around the arm itself is counted.
import math
import os
import maya.cmds as cmds
import maya.mel as mel
import maya.api.OpenMaya as om

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
BASE_DIR = 'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'


# measured in the chest frame, so turning the whole upper body does not count as the arm twisting
def chest_inverse():
    return om.MMatrix(cmds.xform(NS + "Spine2", q=True, ws=True, matrix=True)).inverse()


def ws(j):
    p = om.MPoint(*cmds.xform(NS + j, q=True, ws=True, t=True)) * chest_inverse()
    return om.MVector(p.x, p.y, p.z)


def perpendicular(v, axis, min_length):
    a = axis.normal()
    p = v - a * (v * a)
    return p.normal() if p.length() > min_length else None


def angle(a, b):
    if a is None or b is None:
        return 0.0
    return math.degrees(math.acos(max(-1.0, min(1.0, a * b))))


for clip in os.environ['ROLL_CLIPS'].split(','):
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, clip))
    times = set()
    for c in cmds.ls(type='animCurve'):
        times.update(cmds.keyframe(c, q=True, timeChange=True) or [])
    times = sorted(times)
    for side in ('Right', 'Left'):
        prev_swivel = prev_roll = None
        measured = 0
        worst_swivel = (0.0, 0)
        worst_roll = (0.0, 0)
        for t in times:
            cmds.currentTime(t)
            s, e, h = ws(side + 'Arm'), ws(side + 'ForeArm'), ws(side + 'Hand')
            # an almost straight arm has no defined elbow direction (under 4 cm off the line): skip it
            swivel = perpendicular(e - s, h - s, 4.0)
            m = om.MMatrix(cmds.xform(NS + side + 'Hand', q=True, ws=True, matrix=True))
            hand_side = om.MVector(m[8], m[9], m[10]) * chest_inverse()  # hand local Z axis
            roll = perpendicular(hand_side, h - e, 0.3)
            if prev_swivel is not None and swivel is not None:
                measured += 1
                # carry last frame's directions along with the swing of the arm (the smallest rotation
                # from last frame's axis to this one), so only turning around the arm itself is left
                swing = om.MQuaternion(prev_axis, h - s)
                ds = angle(swivel, prev_swivel.rotateBy(swing))
                fswing = om.MQuaternion(prev_fore, h - e)
                dr = angle(roll, prev_roll.rotateBy(fswing) if prev_roll is not None else None)
                if ds > worst_swivel[0]:
                    worst_swivel = (ds, int(t))
                if dr > worst_roll[0]:
                    worst_roll = (dr, int(t))
                if os.environ.get("ROLL_VERBOSE"):
                    bent = (e - s) - (h - s).normal() * ((e - s) * (h - s).normal())
                    print("RESULT   %s f=%2d swivel %5.1f roll %5.1f bend %4.1fcm" % (side, int(t), ds, dr, bent.length()))
            prev_swivel, prev_roll = swivel, roll
            prev_axis, prev_fore = h - s, h - e
        print('RESULT %-18s %-5s elbow swivel max %5.1f deg/frame (f=%2d)   wrist roll max %5.1f deg/frame (f=%2d)   measured %d/%d' % (
            clip, side, worst_swivel[0], worst_swivel[1], worst_roll[0], worst_roll[1], measured, len(times) - 1))
