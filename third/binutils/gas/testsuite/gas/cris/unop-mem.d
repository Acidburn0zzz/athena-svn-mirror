#objdump: -dr
#name: @OC@

# Test the @OC@ insn.

.*:     file format .*-cris

Disassembly of section \.text:

00000000 <notstart>:
   0:	0000[ ]+	bcc ( 0x2|\.\+2)
	\.\.\.

00000004 <start>:
[ ]+4:[	 ]+@IM+703b@[ 	]+@OC@[ ]+\[r0\]
[ ]+6:[	 ]+@IM+733b@[ 	]+@OC@[ ]+\[r3\]
[ ]+8:[	 ]+@IM+743f@[ 	]+@OC@[ ]+\[r4\+\]
[ ]+a:[	 ]+@IM+713f@[ 	]+@OC@[ ]+\[r1\+\]
[ ]+c:[	 ]+4bd5 @IM+703b@[ 	]+@OC@[ ]+\[r11\+r13\.b\]
[ ]+10:[	 ]+42a5 @IM+703b@[ 	]+@OC@[ ]+\[r2\+r10\.b\]
[ ]+14:[	 ]+45c9 @IM+703b@[ 	]+@OC@[ ]+\[r12\+\[r5\]\.b\]
[ ]+18:[	 ]+4ad9 @IM+703b@[ 	]+@OC@[ ]+\[r13\+\[r10\]\.b\]
[ ]+1c:[	 ]+4d2d @IM+703b@[ 	]+@OC@[ ]+\[r2\+\[r13\+\]\.b\]
[ ]+20:[	 ]+40cd @IM+703b@[ 	]+@OC@[ ]+\[r12\+\[r0\+\]\.b\]
[ ]+24:[	 ]+55b5 @IM+703b@[ 	]+@OC@[ ]+\[r5\+r11\.w\]
[ ]+28:[	 ]+5115 @IM+703b@[ 	]+@OC@[ ]+\[r1\+r1\.w\]
[ ]+2c:[	 ]+5009 @IM+703b@[ 	]+@OC@[ ]+\[r0\+\[r0\]\.w\]
[ ]+30:[	 ]+5729 @IM+703b@[ 	]+@OC@[ ]+\[r2\+\[r7\]\.w\]
[ ]+34:[	 ]+532d @IM+703b@[ 	]+@OC@[ ]+\[r2\+\[r3\+\]\.w\]
[ ]+38:[	 ]+587d @IM+703b@[ 	]+@OC@[ ]+\[r7\+\[r8\+\]\.w\]
[ ]+3c:[	 ]+6255 @IM+703b@[ 	]+@OC@[ ]+\[r2\+r5\.d\]
[ ]+40:[	 ]+63a5 @IM+703b@[ 	]+@OC@[ ]+\[r3\+r10\.d\]
[ ]+44:[	 ]+6259 @IM+703b@[ 	]+@OC@[ ]+\[r5\+\[r2\]\.d\]
[ ]+48:[	 ]+6ac9 @IM+703b@[ 	]+@OC@[ ]+\[r12\+\[r10\]\.d\]
[ ]+4c:[	 ]+651d @IM+703b@[ 	]+@OC@[ ]+\[r1\+\[r5\+\]\.d\]
[ ]+50:[	 ]+6a2d @IM+703b@[ 	]+@OC@[ ]+\[r2\+\[r10\+\]\.d\]
[ ]+54:[	 ]+0021 @IM+703b@[ 	]+@OC@[ ]+\[r2\+0\]
[ ]+58:[	 ]+0121 @IM+703b@[ 	]+@OC@[ ]+\[r2\+1\]
[ ]+5c:[	 ]+7f21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+127\]
[ ]+60:[	 ]+5f2d 8000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+128\]
[ ]+66:[	 ]+ff21 @IM+703b@[ 	]+@OC@[ ]+\[r2-1\]
[ ]+6a:[	 ]+ff21 @IM+703b@[ 	]+@OC@[ ]+\[r2-1\]
[ ]+6e:[	 ]+8121 @IM+703b@[ 	]+@OC@[ ]+\[r2-127\]
[ ]+72:[	 ]+8021 @IM+703b@[ 	]+@OC@[ ]+\[r2-128\]
[ ]+76:[	 ]+8121 @IM+703b@[ 	]+@OC@[ ]+\[r2-127\]
[ ]+7a:[	 ]+8021 @IM+703b@[ 	]+@OC@[ ]+\[r2-128\]
[ ]+7e:[	 ]+5f2d ff00 @IM+703b@[ 	]+@OC@[ ]+\[r2\+255\]
[ ]+84:[	 ]+5f2d 01ff @IM+703b@[ 	]+@OC@[ ]+\[r2-255\]
[ ]+8a:[	 ]+5f2d 01ff @IM+703b@[ 	]+@OC@[ ]+\[r2-255\]
[ ]+90:[	 ]+5f2d 0001 @IM+703b@[ 	]+@OC@[ ]+\[r2\+256\]
[ ]+96:[	 ]+5f2d 00ff @IM+703b@[ 	]+@OC@[ ]+\[r2-256\]
[ ]+9c:[	 ]+5f2d 68dd @IM+703b@[ 	]+@OC@[ ]+\[r2-8856\]
[ ]+a2:[	 ]+5f2d 00ff @IM+703b@[ 	]+@OC@[ ]+\[r2-256\]
[ ]+a8:[	 ]+5f2d 68dd @IM+703b@[ 	]+@OC@[ ]+\[r2-8856\]
[ ]+ae:[	 ]+5f2d 9822 @IM+703b@[ 	]+@OC@[ ]+\[r2\+8856\]
[ ]+b4:[	 ]+6f2d ac72 2a00 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(2781868|2a72ac <two701867\+0x13881>)\]
[ ]+bc:[	 ]+6f2d d5c5 d6ff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+c4:[	 ]+6f2d acce c09e @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x9ec0ceac|9ec0ceac <const_int_m32\+0x70281ef7>)\]
[ ]+cc:[	 ]+6f2d 5331 3f81 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x813f3153|813f3153 <const_int_m32\+0x52a6819e>)\]
[ ]+d4:[	 ]+6f2d 5331 3f81 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x813f3153|813f3153 <const_int_m32\+0x52a6819e>)\]
[ ]+dc:[	 ]+6f2d b5af 982e @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x2e98afb5|2e98afb5 <const_int_m32>)\]
[ ]+e4:[	 ]+6f2d 2b45 941b @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x1b94452b|1b94452b <const_int_32>)\]
[ ]+ec:[	 ]+2a21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+42\]
[ ]+f0:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+f4:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+f8:[	 ]+2a21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+42\]
[ ]+fc:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+100:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+104:[	 ]+2a21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+42\]
[ ]+108:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+10c:[	 ]+2a21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+42\]
[ ]+110:[	 ]+5f2d ff7f @IM+703b@[ 	]+@OC@[ ]+\[r2\+32767\]
[ ]+116:[	 ]+6f2d 0080 0000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(32768|8000 <three2767\+0x1>)\]
[ ]+11e:[	 ]+6f2d 0180 0000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(32769|8001 <three2767\+0x2>)\]
[ ]+126:[	 ]+5f2d 0180 @IM+703b@[ 	]+@OC@[ ]+\[r2-32767\]
[ ]+12c:[	 ]+5f2d 0080 @IM+703b@[ 	]+@OC@[ ]+\[r2-32768\]
[ ]+132:[	 ]+6f2d ff7f ffff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffff7fff|ffff7fff <const_int_m32\+0xd166d04a>)\]
[ ]+13a:[	 ]+5f2d 0180 @IM+703b@[ 	]+@OC@[ ]+\[r2-32767\]
[ ]+140:[	 ]+5f2d 0080 @IM+703b@[ 	]+@OC@[ ]+\[r2-32768\]
[ ]+146:[	 ]+6f2d ff7f ffff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffff7fff|ffff7fff <const_int_m32\+0xd166d04a>)\]
[ ]+14e:[	 ]+6f2d ffff 0000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(65535|ffff <six5535>)\]
[ ]+156:[	 ]+6f2d 0000 0100 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(65536|10000 <six5535\+0x1>)\]
[ ]+15e:[	 ]+6f2d 2b3a 2900 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(2701867|293a2b <two701867>)\]
[ ]+166:[	 ]+6f2d d5c5 d6ff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+16e:[	 ]+6f2d d5c5 d6ff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+176:[	 ]+6f2d 0000 0000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+0( <notstart>)?\]
[ 	]+178:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[ ]+17e:[	 ]+0021 @IM+703b@[ 	]+@OC@[ ]+\[r2\+0\]
[ ]+182:[	 ]+0121 @IM+703b@[ 	]+@OC@[ ]+\[r2\+1\]
[ ]+186:[	 ]+7f21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+127\]
[ ]+18a:[	 ]+5f2d 8000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+128\]
[ ]+190:[	 ]+ff21 @IM+703b@[ 	]+@OC@[ ]+\[r2-1\]
[ ]+194:[	 ]+ff21 @IM+703b@[ 	]+@OC@[ ]+\[r2-1\]
[ ]+198:[	 ]+8121 @IM+703b@[ 	]+@OC@[ ]+\[r2-127\]
[ ]+19c:[	 ]+8021 @IM+703b@[ 	]+@OC@[ ]+\[r2-128\]
[ ]+1a0:[	 ]+8121 @IM+703b@[ 	]+@OC@[ ]+\[r2-127\]
[ ]+1a4:[	 ]+8021 @IM+703b@[ 	]+@OC@[ ]+\[r2-128\]
[ ]+1a8:[	 ]+5f2d ff00 @IM+703b@[ 	]+@OC@[ ]+\[r2\+255\]
[ ]+1ae:[	 ]+5f2d 01ff @IM+703b@[ 	]+@OC@[ ]+\[r2-255\]
[ ]+1b4:[	 ]+5f2d 01ff @IM+703b@[ 	]+@OC@[ ]+\[r2-255\]
[ ]+1ba:[	 ]+5f2d 0001 @IM+703b@[ 	]+@OC@[ ]+\[r2\+256\]
[ ]+1c0:[	 ]+5f2d 00ff @IM+703b@[ 	]+@OC@[ ]+\[r2-256\]
[ ]+1c6:[	 ]+5f2d 68dd @IM+703b@[ 	]+@OC@[ ]+\[r2-8856\]
[ ]+1cc:[	 ]+5f2d 00ff @IM+703b@[ 	]+@OC@[ ]+\[r2-256\]
[ ]+1d2:[	 ]+5f2d 68dd @IM+703b@[ 	]+@OC@[ ]+\[r2-8856\]
[ ]+1d8:[	 ]+5f2d 9822 @IM+703b@[ 	]+@OC@[ ]+\[r2\+8856\]
[ ]+1de:[	 ]+6f2d ac72 2a00 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(2781868|2a72ac <two701867\+0x13881>)\]
[ ]+1e6:[	 ]+6f2d d5c5 d6ff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+1ee:[	 ]+6f2d acce c09e @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x9ec0ceac|9ec0ceac <const_int_m32\+0x70281ef7>)\]
[ ]+1f6:[	 ]+6f2d 5331 3f81 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x813f3153|813f3153 <const_int_m32\+0x52a6819e>)\]
[ ]+1fe:[	 ]+6f2d 5331 3f81 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x813f3153|813f3153 <const_int_m32\+0x52a6819e>)\]
[ ]+206:[	 ]+6f2d b5af 982e @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x2e98afb5|2e98afb5 <const_int_m32>)\]
[ ]+20e:[	 ]+6f2d 2b45 941b @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0x1b94452b|1b94452b <const_int_32>)\]
[ ]+216:[	 ]+2a21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+42\]
[ ]+21a:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+21e:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+222:[	 ]+2a21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+42\]
[ ]+226:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+22a:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+22e:[	 ]+2a21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+42\]
[ ]+232:[	 ]+d621 @IM+703b@[ 	]+@OC@[ ]+\[r2-42\]
[ ]+236:[	 ]+2a21 @IM+703b@[ 	]+@OC@[ ]+\[r2\+42\]
[ ]+23a:[	 ]+5f2d ff7f @IM+703b@[ 	]+@OC@[ ]+\[r2\+32767\]
[ ]+240:[	 ]+6f2d 0080 0000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(32768|8000 <three2767\+0x1>)\]
[ ]+248:[	 ]+6f2d 0180 0000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(32769|8001 <three2767\+0x2>)\]
[ ]+250:[	 ]+5f2d 0180 @IM+703b@[ 	]+@OC@[ ]+\[r2-32767\]
[ ]+256:[	 ]+5f2d 0080 @IM+703b@[ 	]+@OC@[ ]+\[r2-32768\]
[ ]+25c:[	 ]+6f2d ff7f ffff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffff7fff|ffff7fff <const_int_m32\+0xd166d04a>)\]
[ ]+264:[	 ]+5f2d 0180 @IM+703b@[ 	]+@OC@[ ]+\[r2-32767\]
[ ]+26a:[	 ]+5f2d 0080 @IM+703b@[ 	]+@OC@[ ]+\[r2-32768\]
[ ]+270:[	 ]+6f2d ff7f ffff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffff7fff|ffff7fff <const_int_m32\+0xd166d04a>)\]
[ ]+278:[	 ]+6f2d ffff 0000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(65535|ffff <six5535>)\]
[ ]+280:[	 ]+6f2d 0000 0100 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(65536|10000 <six5535\+0x1>)\]
[ ]+288:[	 ]+6f2d 2b3a 2900 @IM+703b@[ 	]+@OC@[ ]+\[r2\+(2701867|293a2b <two701867>)\]
[ ]+290:[	 ]+6f2d d5c5 d6ff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+298:[	 ]+6f2d d5c5 d6ff @IM+703b@[ 	]+@OC@[ ]+\[r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+2a0:[	 ]+6f2d 0000 0000 @IM+703b@[ 	]+@OC@[ ]+\[r2\+0( <notstart>)?\]
[ 	]+2a2:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[ ]+2a8:[	 ]+4235 @IM+713f@[ 	]+@OC@[ ]+\[r1=r2\+r3\.b\]
[ ]+2ac:[	 ]+42a5 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+r10\.b\]
[ ]+2b0:[	 ]+4529 @IM+703f@[ 	]+@OC@[ ]+\[r0=r2\+\[r5\]\.b\]
[ ]+2b4:[	 ]+4a29 @IM+733f@[ 	]+@OC@[ ]+\[r3=r2\+\[r10\]\.b\]
[ ]+2b8:[	 ]+442d @IM+753f@[ 	]+@OC@[ ]+\[r5=r2\+\[r4\+\]\.b\]
[ ]+2bc:[	 ]+474d @IM+723f@[ 	]+@OC@[ ]+\[r2=r4\+\[r7\+\]\.b\]
[ ]+2c0:[	 ]+5c55 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r12\+r5\.w\]
[ ]+2c4:[	 ]+53a5 @IM+713f@[ 	]+@OC@[ ]+\[r1=r3\+r10\.w\]
[ ]+2c8:[	 ]+5529 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+\[r5\]\.w\]
[ ]+2cc:[	 ]+5a79 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r7\+\[r10\]\.w\]
[ ]+2d0:[	 ]+576d @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r6\+\[r7\+\]\.w\]
[ ]+2d4:[	 ]+513d @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r3\+\[r1\+\]\.w\]
[ ]+2d8:[	 ]+6255 @IM+743f@[ 	]+@OC@[ ]+\[r4=r2\+r5\.d\]
[ ]+2dc:[	 ]+62a5 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+r10\.d\]
[ ]+2e0:[	 ]+6539 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r3\+\[r5\]\.d\]
[ ]+2e4:[	 ]+6a49 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r4\+\[r10\]\.d\]
[ ]+2e8:[	 ]+658d @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r8\+\[r5\+\]\.d\]
[ ]+2ec:[	 ]+6a9d @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r9\+\[r10\+\]\.d\]
[ ]+2f0:[	 ]+0021 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+0\]
[ ]+2f4:[	 ]+0121 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+1\]
[ ]+2f8:[	 ]+7f21 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+127\]
[ ]+2fc:[	 ]+5f2d 8000 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+128\]
[ ]+302:[	 ]+ff21 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-1\]
[ ]+306:[	 ]+ff21 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-1\]
[ ]+30a:[	 ]+8121 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-127\]
[ ]+30e:[	 ]+8021 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-128\]
[ ]+312:[	 ]+8121 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-127\]
[ ]+316:[	 ]+8021 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-128\]
[ ]+31a:[	 ]+5f2d ff00 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+255\]
[ ]+320:[	 ]+5f2d 01ff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-255\]
[ ]+326:[	 ]+5f2d 01ff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-255\]
[ ]+32c:[	 ]+5f2d 0001 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+256\]
[ ]+332:[	 ]+5f2d 00ff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-256\]
[ ]+338:[	 ]+5f2d 68dd @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-8856\]
[ ]+33e:[	 ]+5f2d 00ff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-256\]
[ ]+344:[	 ]+5f2d 68dd @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-8856\]
[ ]+34a:[	 ]+5f2d 9822 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+8856\]
[ ]+350:[	 ]+6f2d ac72 2a00 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(2781868|2a72ac <two701867\+0x13881>)\]
[ ]+358:[	 ]+6f2d d5c5 d6ff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+360:[	 ]+6f2d acce c09e @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0x9ec0ceac|9ec0ceac <const_int_m32\+0x70281ef7>)\]
[ ]+368:[	 ]+6f2d 5331 3f81 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0x813f3153|813f3153 <const_int_m32\+0x52a6819e>)\]
[ ]+370:[	 ]+6f2d 5331 3f81 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0x813f3153|813f3153 <const_int_m32\+0x52a6819e>)\]
[ ]+378:[	 ]+6f2d b5af 982e @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0x2e98afb5|2e98afb5 <const_int_m32>)\]
[ ]+380:[	 ]+6f2d 2b45 941b @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0x1b94452b|1b94452b <const_int_32>)\]
[ ]+388:[	 ]+2a21 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+42\]
[ ]+38c:[	 ]+d621 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-42\]
[ ]+390:[	 ]+d621 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-42\]
[ ]+394:[	 ]+2a21 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+42\]
[ ]+398:[	 ]+d621 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-42\]
[ ]+39c:[	 ]+d621 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-42\]
[ ]+3a0:[	 ]+2a21 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+42\]
[ ]+3a4:[	 ]+d621 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-42\]
[ ]+3a8:[	 ]+2a21 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+42\]
[ ]+3ac:[	 ]+5f2d ff7f @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+32767\]
[ ]+3b2:[	 ]+6f2d 0080 0000 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(32768|8000 <three2767\+0x1>)\]
[ ]+3ba:[	 ]+6f2d 0180 0000 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(32769|8001 <three2767\+0x2>)\]
[ ]+3c2:[	 ]+5f2d 0180 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-32767\]
[ ]+3c8:[	 ]+5f2d 0080 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-32768\]
[ ]+3ce:[	 ]+6f2d ff7f ffff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0xffff7fff|ffff7fff <const_int_m32\+0xd166d04a>)\]
[ ]+3d6:[	 ]+5f2d 0180 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-32767\]
[ ]+3dc:[	 ]+5f2d 0080 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2-32768\]
[ ]+3e2:[	 ]+6f2d ff7f ffff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0xffff7fff|ffff7fff <const_int_m32\+0xd166d04a>)\]
[ ]+3ea:[	 ]+6f2d ffff 0000 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(65535|ffff <six5535>)\]
[ ]+3f2:[	 ]+6f2d 0000 0100 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(65536|10000 <six5535\+0x1>)\]
[ ]+3fa:[	 ]+6f2d 2b3a 2900 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(2701867|293a2b <two701867>)\]
[ ]+402:[	 ]+6f2d d5c5 d6ff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+40a:[	 ]+6f2d d5c5 d6ff @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+(0xffd6c5d5|ffd6c5d5 <const_int_m32\+0xd13e1620>)\]
[ ]+412:[	 ]+6f2d 0000 0000 @IM+7c3f@[ 	]+@OC@[ ]+\[r12=r2\+0( <notstart>)?\]
[ 	]+414:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[ ]+41a:[	 ]+7209 @IM+703b@[ 	]+@OC@[ ]+\[\[r2\]\]
[ ]+41e:[	 ]+7309 @IM+703b@[ 	]+@OC@[ ]+\[\[r3\]\]
[ ]+422:[	 ]+730d @IM+703b@[ 	]+@OC@[ ]+\[\[r3\+\]\]
[ ]+426:[	 ]+710d @IM+703b@[ 	]+@OC@[ ]+\[\[r1\+\]\]
[ ]+42a:[	 ]+7f0d 0000 0000 @IM+703b@[ 	]+@OC@[ ]+\[(0x0|0 <notstart>)\]
[ 	]+42c:[ 	]+(R_CRIS_)?32[ 	]+externalsym
[ ]+432:[	 ]+7f0d 0000 0000 @IM+703b@[ 	]+@OC@[ ]+\[(0x0|0 <notstart>)\]
[ 	]+434:[ 	]+(R_CRIS_)?32[ 	]+\.text

0000043a <end>:
	\.\.\.
