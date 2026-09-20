# Measure how much each leg turns around itself from one frame to the next.
#   knee swivel : where the knee points around the hip-ankle line (swing of the leg taken out)
#   foot turn   : how much the foot turns in the world in one frame
#   foot vs f0  : how far the foot has turned from frame 0 (a planted foot should stay near 0)
#   hips turn   : how fast the whole body turns, to tell a spinning body from a spinning leg
#   thigh/shin twist : how much the thigh or shin bone turns around its own length (swing taken out)
import math
import os
import maya.cmds as cmds
import maya.mel as mel
import maya.api.OpenMaya as om

cmds.loadPlugin('fbxmaya', quiet=True)
NS = 'mixamorig:'
BASE_DIR = 'C:/git/god_syu/god/AZEFIGHTER/Assets/Model/knight/'


def ws(j):
    return om.MVector(*cmds.xform(NS + j, q=True, ws=True, t=True))


def world_rot(j):
    return om.MTransformationMatrix(om.MMatrix(cmds.xform(NS + j, q=True, ws=True, matrix=True))).rotation(asQuaternion=True)


def perpendicular(v, axis, min_length):
    a = axis.normal()
    p = v - a * (v * a)
    return p.normal() if p.length() > min_length else None


def angle(a, b):
    if a is None or b is None:
        return 0.0
    return math.degrees(math.acos(max(-1.0, min(1.0, a * b))))


ACROSS_ROW = {}


def bone_roll(j, child):
    """A bone's local axis that lies across the bone, to see it turn around its own length.
    The axis is picked once per bone (the one most across it on the first frame) and kept."""
    m = om.MMatrix(cmds.xform(NS + j, q=True, ws=True, matrix=True))
    along = (ws(child) - ws(j)).normal()
    rows = [om.MVector(m[r * 4], m[r * 4 + 1], m[r * 4 + 2]).normal() for r in range(3)]
    if j not in ACROSS_ROW:
        ACROSS_ROW[j] = min(range(3), key=lambda r: abs(rows[r] * along))
    return perpendicular(rows[ACROSS_ROW[j]], along, 0.3), along


def quat_angle(a, b):
    d = abs(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w)
    return math.degrees(2.0 * math.acos(min(1.0, d)))


for clip in os.environ['LEG_CLIPS'].split(','):
    cmds.file(new=True, force=True)
    mel.eval('FBXImport -f "%s%s.fbx"' % (BASE_DIR, clip))
    ACROSS_ROW.clear()
    times = sorted(set(sum([cmds.keyframe(c, q=True, timeChange=True) or [] for c in cmds.ls(type='animCurve')], [])))
    prev_hips, worst_hips = None, (0.0, 0)
    for t in times:
        cmds.currentTime(t)
        q = world_rot('Hips')
        if prev_hips is not None and quat_angle(q, prev_hips) > worst_hips[0]:
            worst_hips = (quat_angle(q, prev_hips), int(t))
        prev_hips = q
    print('RESULT %-18s hips  turn max %5.1f/f (f=%2d)' % (clip, worst_hips[0], worst_hips[1]))
    for side in ('Right', 'Left'):
        prev = None
        worst_knee = worst_turn = worst_f0 = (0.0, 0)
        worst_bone = (0.0, 0)
        prev_bones = None
        first_rot = None
        for t in times:
            cmds.currentTime(t)
            # knee measured in the hips frame, so turning the whole body does not count as the leg twisting
            hips_inv = om.MMatrix(cmds.xform(NS + 'Hips', q=True, ws=True, matrix=True)).inverse()
            h, k, a = (om.MVector(*(lambda p: (p.x, p.y, p.z))(om.MPoint(ws(side + j)) * hips_inv)) for j in ('UpLeg', 'Leg', 'Foot'))
            # an almost straight leg has no defined knee direction (under 4 cm off the line): skip it
            knee = perpendicular(k - h, a - h, 4.0)
            rot = world_rot(side + 'Foot')
            if first_rot is None:
                first_rot = rot
            if prev is not None:
                p_knee, p_axis, p_rot = prev
                dk = angle(knee, p_knee.rotateBy(om.MQuaternion(p_axis, a - h)) if p_knee is not None else None)
                dt = quat_angle(rot, p_rot)
                if dk > worst_knee[0]:
                    worst_knee = (dk, int(t))
                if dt > worst_turn[0]:
                    worst_turn = (dt, int(t))
                if os.environ.get('LEG_VERBOSE'):
                    bent = (k - h) - (a - h).normal() * ((k - h) * (a - h).normal())
                    print('RESULT   %s f=%2d knee %5.1f foot %5.1f vs f0 %5.1f bend %4.1fcm' % (side, int(t), dk, dt, quat_angle(rot, first_rot), bent.length()))
            f0 = quat_angle(rot, first_rot)
            if f0 > worst_f0[0]:
                worst_f0 = (f0, int(t))
            bones = [bone_roll(side + 'UpLeg', side + 'Leg'), bone_roll(side + 'Leg', side + 'Foot')]
            if prev_bones is not None:
                for (r, al), (pr, pal) in zip(bones, prev_bones):
                    db = angle(r, pr.rotateBy(om.MQuaternion(pal, al)))
                    if db > worst_bone[0]:
                        worst_bone = (db, int(t))
            prev_bones = bones
            prev = (knee, a - h, rot)
        print('RESULT %-18s %-5s thigh/shin twist max %5.1f/f (f=%2d)' % (clip, side, worst_bone[0], worst_bone[1]))
        print('RESULT %-18s %-5s knee swivel max %5.1f/f (f=%2d)  foot turn max %5.1f/f (f=%2d)  foot vs f0 max %5.1f (f=%2d)' % (
            clip, side, worst_knee[0], worst_knee[1], worst_turn[0], worst_turn[1], worst_f0[0], worst_f0[1]))
