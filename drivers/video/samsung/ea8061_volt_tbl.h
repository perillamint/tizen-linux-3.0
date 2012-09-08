#ifndef __REF_VOLT_TABLE_H__
#define __REF_VOLT_TABLE_H__


static u32 volt_table_vt[16] = {
	399769600,	391840286,	383910971,	375981657,
	368052342,	360123028,	352193714,	344264399,
	336335085,	328405771,	308582485,	301974723,
	295366961,	288759199,	282151437,	276865227,
};

static u32 volt_table_v255[505] = {
	369373895,	368713119,	368052342,	367391566,	366730790,	366070014,	365409238,	364748461,
	364087685,	363426909,	362766133,	362105357,	361444580,	360783804,	360123028,	359462252,
	358801476,	358140700,	357479923,	356819147,	356158371,	355497595,	354836819,	354176042,
	353515266,	352854490,	352193714,	351532938,	350872161,	350211385,	349550609,	348889833,
	348229057,	347568280,	346907504,	346246728,	345585952,	344925176,	344264399,	343603623,
	342942847,	342282071,	341621295,	340960518,	340299742,	339638966,	338978190,	338317414,
	337656637,	336995861,	336335085,	335674309,	335013533,	334352756,	333691980,	333031204,
	332370428,	331709652,	331048875,	330388099,	329727323,	329066547,	328405771,	327744994,
	327084218,	326423442,	325762666,	325101890,	324441113,	323780337,	323119561,	322458785,
	321798009,	321137232,	320476456,	319815680,	319154904,	318494128,	317833351,	317172575,
	316511799,	315851023,	315190247,	314529470,	313868694,	313207918,	312547142,	311886366,
	311225589,	310564813,	309904037,	309243261,	308582485,	307921708,	307260932,	306600156,
	305939380,	305278604,	304617827,	303957051,	303296275,	302635499,	301974723,	301313946,
	300653170,	299992394,	299331618,	298670842,	298010065,	297349289,	296688513,	296027737,
	295366961,	294706184,	294045408,	293384632,	292723856,	292063080,	291402303,	290741527,
	290080751,	289419975,	288759199,	288098422,	287437646,	286776870,	286116094,	285455318,
	284794541,	284133765,	283472989,	282812213,	282151437,	281490660,	280829884,	280169108,
	279508332,	278847556,	278186780,	277526003,	276865227,	276204451,	275543675,	274882899,
	274222122,	273561346,	272900570,	272239794,	271579018,	270918241,	270257465,	269596689,
	268935913,	268275137,	267614360,	266953584,	266292808,	265632032,	264971256,	264310479,
	263649703,	262988927,	262328151,	261667375,	261006598,	260345822,	259685046,	259024270,
	258363494,	257702717,	257041941,	256381165,	255720389,	255059613,	254398836,	253738060,
	253077284,	252416508,	251755732,	251094955,	250434179,	249773403,	249112627,	248451851,
	247791074,	247130298,	246469522,	245808746,	245147970,	244487193,	243826417,	243165641,
	242504865,	241844089,	241183312,	240522536,	239861760,	239200984,	238540208,	237879431,
	237218655,	236557879,	235897103,	235236327,	234575550,	233914774,	233253998,	232593222,
	231932446,	231271669,	230610893,	229950117,	229289341,	228628565,	227967788,	227307012,
	226646236,	225985460,	225324684,	224663907,	224003131,	223342355,	222681579,	222020803,
	221360026,	220699250,	220038474,	219377698,	218716922,	218056145,	217395369,	216734593,
	216073817,	215413041,	214752264,	214091488,	213430712,	212769936,	212109160,	211448383,
	210787607,	210126831,	209466055,	208805279,	208144502,	207483726,	206822950,	206162174,
	205501398,	204840621,	204179845,	203519069,	202858293,	202197517,	201536740,	200875964,
	200215188,	199554412,	198893636,	198232860,	197572083,	196911307,	196250531,	195589755,
	194928979,	194268202,	193607426,	192946650,	192285874,	191625098,	190964321,	190303545,
	189642769,	188981993,	188321217,	187660440,	186999664,	186338888,	185678112,	185017336,
	184356559,	183695783,	183035007,	182374231,	181713455,	181052678,	180391902,	179731126,
	179070350,	178409574,	177748797,	177088021,	176427245,	175766469,	175105693,	174444916,
	173784140,	173123364,	172462588,	171801812,	171141035,	170480259,	169819483,	169158707,
	168497931,	167837154,	167176378,	166515602,	165854826,	165194050,	164533273,	163872497,
	163211721,	162550945,	161890169,	161229392,	160568616,	159907840,	159247064,	158586288,
	157925511,	157264735,	156603959,	155943183,	155282407,	154621630,	153960854,	153300078,
	152639302,	151978526,	151317749,	150656973,	149996197,	149335421,	148674645,	148013868,
	147353092,	146692316,	146031540,	145370764,	144709987,	144049211,	143388435,	142727659,
	142066883,	141406106,	140745330,	140084554,	139423778,	138763002,	138102225,	137441449,
	136780673,	136119897,	135459121,	134798344,	134137568,	133476792,	132816016,	132155240,
	131494463,	130833687,	130172911,	129512135,	128851359,	128190582,	127529806,	126869030,
	126208254,	125547478,	124886701,	124225925,	123565149,	122904373,	122243597,	121582820,
	120922044,	120261268,	119600492,	118939716,	118278940,	117618163,	116957387,	116296611,
	115635835,	114975059,	114314282,	113653506,	112992730,	112331954,	111671178,	111010401,
	110349625,	109688849,	109028073,	108367297,	107706520,	107045744,	106384968,	105724192,
	105063416,	104402639,	103741863,	103081087,	102420311,	101759535,	101098758,	100437982,
	99777206,	99116430,	98455654,	97794877,	97134101,	96473325,	95812549,	95151773,
	94490996,	93830220,	93169444,	92508668,	91847892,	91187115,	90526339,	89865563,
	89204787,	88544011,	87883234,	87222458,	86561682,	85900906,	85240130,	84579353,
	83918577,	83257801,	82597025,	81936249,	81275472,	80614696,	79953920,	79293144,
	78632368,	77971591,	77310815,	76650039,	75989263,	75328487,	74667710,	74006934,
	73346158,	72685382,	72024606,	71363829,	70703053,	70042277,	69381501,	68720725,
	68059948,	67399172,	66738396,	66077620,	65416844,	64756067,	64095291,	63434515,
	62773739,	62112963,	61452186,	60791410,	60130634,	59469858,	58809082,	58148305,
	57487529,	56826753,	56165977,	55505201,	54844424,	54183648,	53522872,	52862096,
	52201320,	51540543,	50879767,	50218991,	49558215,	48897439,	48236662,	47575886,
	46915110,	46254334,	45593558,	44932781,	44272005,	43611229,	42950453,	42289677,
	41628900,	40968124,	40307348,	39646572,	38985796,	38325020,	37664243,	37003467,
	36342691
};

static u32 volt_table_cv_64_dv_320[256] = {
	13107,	13312,	13516,	13721,	13926,	14131,	14336,	14540,
	14745,	14950,	15155,	15360,	15564,	15769,	15974,	16179,
	16384,	16588,	16793,	16998,	17203,	17408,	17612,	17817,
	18022,	18227,	18432,	18636,	18841,	19046,	19251,	19456,
	19660,	19865,	20070,	20275,	20480,	20684,	20889,	21094,
	21299,	21504,	21708,	21913,	22118,	22323,	22528,	22732,
	22937,	23142,	23347,	23552,	23756,	23961,	24166,	24371,
	24576,	24780,	24985,	25190,	25395,	25600,	25804,	26009,
	26214,	26419,	26624,	26828,	27033,	27238,	27443,	27648,
	27852,	28057,	28262,	28467,	28672,	28876,	29081,	29286,
	29491,	29696,	29900,	30105,	30310,	30515,	30720,	30924,
	31129,	31334,	31539,	31744,	31948,	32153,	32358,	32563,
	32768,	32972,	33177,	33382,	33587,	33792,	33996,	34201,
	34406,	34611,	34816,	35020,	35225,	35430,	35635,	35840,
	36044,	36249,	36454,	36659,	36864,	37068,	37273,	37478,
	37683,	37888,	38092,	38297,	38502,	38707,	38912,	39116,
	39321,	39526,	39731,	39936,	40140,	40345,	40550,	40755,
	40960,	41164,	41369,	41574,	41779,	41984,	42188,	42393,
	42598,	42803,	43008,	43212,	43417,	43622,	43827,	44032,
	44236,	44441,	44646,	44851,	45056,	45260,	45465,	45670,
	45875,	46080,	46284,	46489,	46694,	46899,	47104,	47308,
	47513,	47718,	47923,	48128,	48332,	48537,	48742,	48947,
	49152,	49356,	49561,	49766,	49971,	50176,	50380,	50585,
	50790,	50995,	51200,	51404,	51609,	51814,	52019,	52224,
	52428,	52633,	52838,	53043,	53248,	53452,	53657,	53862,
	54067,	54272,	54476,	54681,	54886,	55091,	55296,	55500,
	55705,	55910,	56115,	56320,	56524,	56729,	56934,	57139,
	57344,	57548,	57753,	57958,	58163,	58368,	58572,	58777,
	58982,	59187,	59392,	59596,	59801,	60006,	60211,	60416,
	60620,	60825,	61030,	61235,	61440,	61644,	61849,	62054,
	62259,	62464,	62668,	62873,	63078,	63283,	63488,	63692,
	63897,	64102,	64307,	64512,	64716,	64921,	65126,	65331,
};

static const u32 gamma_300_gra_table[256] = {
	0,	    2,	    7,	    17,
	32,	    53,	    78,	    110,
	148,	191,	241,	298,
	361,	430,	506,	589,
	679,	776,	880,	991,
	1109,	1235,	1368,	1508,
	1657,	1812,	1975,	2147,
	2325,	2512,	2706,	2909,
	3119,	3338,	3564,	3799,
	4042,	4293,	4553,	4820,
	5096,	5381,	5674,	5975,
	6285,	6604,	6931,	7267,
	7611,	7965,	8327,	8697,
	9077,	9465,	9863,	10269,
	10684,	11109,	11542,	11984,
	12436,	12896,	13366,	13845,
	14333,	14830,	15337,	15852,
	16378,	16912,	17456,	18009,
	18572,	19144,	19726,	20317,
	20918,	21528,	22148,	22778,
	23417,	24066,	24724,	25392,
	26070,	26758,	27456,	28163,
	28880,	29607,	30344,	31090,
	31847,	32613,	33390,	34176,
	34973,	35779,	36596,	37422,
	38259,	39106,	39963,	40830,
	41707,	42594,	43492,	44399,
	45317,	46246,	47184,	48133,
	49092,	50062,	51042,	52032,
	53032,	54043,	55065,	56097,
	57139,	58192,	59255,	60329,
	61413,	62508,	63613,	64729,
	65856,	66993,	68141,	69299,
	70469,	71648,	72839,	74040,
	75252,	76475,	77708,	78952,
	80207,	81473,	82750,	84037,
	85336,	86645,	87965,	89296,
	90638,	91990,	93354,	94729,
	96114,	97511,	98919,	100337,
	101767,	103208,	104659,	106122,
	107596,	109081,	110577,	112085,
	113603,	115132,	116673,	118225,
	119788,	121362,	122948,	124544,
	126152,	127772,	129402,	131044,
	132697,	134361,	136037,	137724,
	139422,	141132,	142853,	144586,
	146330,	148085,	149852,	151630,
	153419,	155220,	157033,	158857,
	160692,	162540,	164398,	166268,
	168150,	170043,	171948,	173864,
	175792,	177731,	179683,	181645,
	183620,	185606,	187603,	189613,
	191634,	193667,	195711,	197767,
	199835,	201915,	204006,	206109,
	208224,	210351,	212489,	214640,
	216802,	218976,	221161,	223359,
	225569,	227790,	230023,	232268,
	234525,	236794,	239075,	241368,
	243672,	245989,	248318,	250658,
	253011,	255375,	257752,	260141,
	262541,	264954,	267379,	269815,
	272264,	274725,	277198,	279683,
	282180,	284689,	287211,	289744,
	292290,	294848,	297418,	300000,
};

static const u32 gamma_22_table[256] = {
	0,	6,	24,	57,	108,	176,	262,	368,
	493,	639,	805,	993,	1202,	1434,	1687,	1964,
	2263,	2586,	2933,	3303,	3698,	4117,	4560,	5029,
	5522,	6041,	6585,	7156,	7752,	8374,	9022,	9697,
	10398,	11127,	11882,	12664,	13474,	14311,	15176,	16068,
	16989,	17937,	18913,	19918,	20952,	22013,	23104,	24223,
	25372,	26549,	27756,	28992,	30257,	31552,	32876,	34231,
	35615,	37029,	38473,	39948,	41452,	42988,	44553,	46149,
	47776,	49434,	51123,	52842,	54593,	56375,	58188,	60032,
	61908,	63815,	65754,	67725,	69728,	71762,	73828,	75927,
	78057,	80220,	82415,	84642,	86902,	89194,	91519,	93876,
	96267,	98690,	101146,	103635,	106157,	108712,	111300,	113921,
	116576,	119265,	121986,	124741,	127530,	130353,	133209,	136099,
	139023,	141981,	144973,	147999,	151059,	154153,	157281,	160444,
	163641,	166873,	170139,	173440,	176775,	180145,	183549,	186989,
	190463,	193973,	197517,	201096,	204711,	208360,	212045,	215765,
	219520,	223311,	227137,	230999,	234896,	238828,	242797,	246801,
	250841,	254916,	259028,	263175,	267359,	271578,	275833,	280125,
	284453,	288816,	293217,	297653,	302126,	306635,	311181,	315763,
	320382,	325037,	329730,	334458,	339224,	344026,	348865,	353741,
	358654,	363604,	368591,	373616,	378677,	383775,	388911,	394084,
	399294,	404541,	409826,	415149,	420508,	425906,	431341,	436813,
	442323,	447871,	453457,	459080,	464742,	470441,	476178,	481953,
	487766,	493617,	499506,	505433,	511398,	517402,	523444,	529524,
	535642,	541799,	547994,	554228,	560500,	566810,	573159,	579547,
	585973,	592439,	598942,	605485,	612066,	618686,	625345,	632043,
	638780,	645556,	652371,	659224,	666117,	673050,	680021,	687031,
	694081,	701170,	708298,	715466,	722673,	729919,	737205,	744531,
	751896,	759300,	766744,	774228,	781751,	789314,	796917,	804560,
	812242,	819964,	827726,	835528,	843370,	851252,	859174,	867136,
	875138,	883180,	891263,	899385,	907548,	915751,	923994,	932277,
	940601,	948965,	957370,	965815,	974301,	982827,	991393,	1000000
};

static const u32 gamma_212_table[256] = {
	0,	7,	34,	81,	149,	239,	353,	489,
	649,	833,	1042,	1276,	1534,	1818,	2127,	2462,
	2824,	3211,	3625,	4065,	4532,	5026,	5547,	6095,
	6670,	7273,	7904,	8563,	9249,	9963,	10706,	11476,
	12275,	13103,	13959,	14844,	15757,	16700,	17671,	18672,
	19701,	20760,	21848,	22966,	24113,	25289,	26496,	27732,
	28997,	30293,	31619,	32974,	34360,	35776,	37222,	38699,
	40206,	41743,	43311,	44909,	46538,	48198,	49889,	51610,
	53362,	55145,	56959,	58804,	60681,	62588,	64527,	66497,
	68498,	70530,	72594,	74690,	76817,	78976,	81166,	83388,
	85641,	87927,	90244,	92593,	94974,	97387,	99832,	102309,
	104818,	107359,	109933,	112538,	115176,	117847,	120549,	123284,
	126052,	128851,	131684,	134549,	137446,	140377,	143339,	146335,
	149363,	152424,	155518,	158645,	161805,	164998,	168223,	171482,
	174774,	178098,	181456,	184847,	188272,	191729,	195220,	198744,
	202301,	205892,	209516,	213173,	216864,	220589,	224347,	228138,
	231963,	235822,	239714,	243640,	247600,	251594,	255621,	259682,
	263777,	267905,	272068,	276265,	280495,	284760,	289058,	293391,
	297757,	302158,	306593,	311062,	315565,	320102,	324674,	329280,
	333920,	338595,	343303,	348047,	352824,	357636,	362483,	367364,
	372279,	377229,	382214,	387233,	392286,	397375,	402498,	407655,
	412848,	418075,	423337,	428633,	433965,	439331,	444732,	450168,
	455639,	461145,	466686,	472262,	477873,	483518,	489199,	494915,
	500666,	506452,	512273,	518130,	524021,	529948,	535910,	541907,
	547940,	554008,	560111,	566249,	572423,	578633,	584877,	591157,
	597473,	603824,	610210,	616632,	623089,	629583,	636111,	642675,
	649275,	655911,	662582,	669288,	676031,	682809,	689623,	696473,
	703358,	710279,	717236,	724229,	731258,	738323,	745423,	752559,
	759732,	766940,	774184,	781465,	788781,	796133,	803522,	810946,
	818407,	825903,	833436,	841005,	848610,	856251,	863928,	871642,
	879392,	887178,	895000,	902859,	910754,	918685,	926653,	934657,
	942697,	950774,	958887,	967037,	975223,	983445,	991704,	1000000
};

static const u32 gamma_21_table[256] = {
	0,	9,	38,	89,	163,	260,	381,	526,
	697,	892,	1113,	1359,	1632,	1930,	2255,	2607,
	2985,	3391,	3823,	4283,	4770,	5284,	5826,	6396,
	6994,	7620,	8274,	8957,	9668,	10407,	11175,	11971,
	12797,	13651,	14534,	15446,	16388,	17358,	18358,	19387,
	20446,	21534,	22652,	23799,	24976,	26183,	27420,	28687,
	29983,	31310,	32667,	34054,	35471,	36919,	38397,	39905,
	41444,	43014,	44614,	46244,	47906,	49598,	51321,	53074,
	54859,	56674,	58521,	60399,	62307,	64247,	66218,	68220,
	70253,	72318,	74414,	76542,	78700,	80891,	83113,	85366,
	87651,	89968,	92316,	94696,	97108,	99551,	102027,	104534,
	107073,	109644,	112248,	114883,	117550,	120249,	122980,	125744,
	128540,	131367,	134228,	137120,	140045,	143002,	145991,	149013,
	152068,	155155,	158274,	161426,	164610,	167827,	171077,	174359,
	177674,	181022,	184403,	187816,	191262,	194741,	198253,	201797,
	205375,	208985,	212629,	216305,	220015,	223758,	227533,	231342,
	235184,	239059,	242967,	246909,	250883,	254891,	258932,	263007,
	267115,	271256,	275431,	279639,	283880,	288155,	292464,	296806,
	301181,	305590,	310033,	314509,	319019,	323562,	328139,	332750,
	337394,	342073,	346785,	351530,	356310,	361123,	365971,	370852,
	375767,	380715,	385698,	390715,	395766,	400851,	405969,	411122,
	416309,	421530,	426785,	432074,	437397,	442754,	448146,	453572,
	459032,	464526,	470054,	475617,	481214,	486845,	492511,	498211,
	503945,	509714,	515517,	521355,	527227,	533133,	539074,	545050,
	551060,	557104,	563183,	569297,	575445,	581628,	587845,	594097,
	600384,	606705,	613061,	619452,	625877,	632338,	638833,	645362,
	651927,	658526,	665161,	671830,	678533,	685272,	692046,	698854,
	705698,	712576,	719490,	726438,	733421,	740440,	747493,	754581,
	761705,	768863,	776057,	783286,	790550,	797848,	805183,	812552,
	819956,	827396,	834871,	842381,	849926,	857506,	865122,	872773,
	880460,	888181,	895938,	903731,	911558,	919421,	927320,	935254,
	943223,	951228,	959268,	967343,	975454,	983601,	991783,	1000000
};

static const struct str_flookup_table flookup_table[302] = {
	{  0,   0},  {  1,  20},
	{ 20,   7},  { 27,   5},
	{ 32,   4},  { 36,   4},
	{ 40,   4},  { 44,   3},
	{ 47,   3},  { 50,   2},
	{ 52,   3},  { 55,   2},
	{ 57,   3},  { 60,   2},
	{ 62,   2},  { 64,   2},
	{ 66,   2},  { 68,   2},
	{ 70,   1},  { 71,   2},
	{ 73,   2},  { 75,   2},
	{ 77,   1},  { 78,   2},
	{ 80,   1},  { 81,   2},
	{ 83,   1},  { 84,   2},
	{ 86,   1},  { 87,   2},
	{ 89,   1},  { 90,   1},
	{ 91,   2},  { 93,   1},
	{ 94,   1},  { 95,   2},
	{ 97,   1},  { 98,   1},
	{ 99,   1},  {100,   1},
	{101,   2},  {103,   1},
	{104,   1},  {105,   1},
	{106,   1},  {107,   1},
	{108,   1},  {109,   1},
	{110,   1},  {111,   1},
	{112,   1},  {113,   1},
	{114,   1},  {115,   1},
	{116,   1},  {117,   1},
	{118,   1},  {119,   1},
	{120,   1},  {121,   1},
	{122,   1},  {123,   1},
	{124,   1},  {125,   1},
	{126,   1},  {127,   1},
	{128,   1},  {129,   1},
	{  0,   0},  {130,   1},
	{131,   1},  {132,   1},
	{133,   1},  {134,   1},
	{  0,   0},  {135,   1},
	{136,   1},  {137,   1},
	{138,   1},  {139,   1},
	{  0,   0},  {140,   1},
	{141,   1},  {142,   1},
	{  0,   0},  {143,   1},
	{144,   1},  {145,   1},
	{146,   1},  {  0,   0},
	{147,   1},  {148,   1},
	{149,   1},  {  0,   0},
	{150,   1},  {151,   1},
	{  0,   0},  {152,   1},
	{153,   1},  {154,   1},
	{  0,   0},  {155,   1},
	{156,   1},  {  0,   0},
	{157,   1},  {158,   1},
	{  0,   0},  {159,   1},
	{160,   1},  {  0,   0},
	{161,   1},  {162,   1},
	{  0,   0},  {163,   1},
	{164,   1},  {  0,   0},
	{165,   1},  {166,   1},
	{  0,   0},  {167,   1},
	{168,   1},  {  0,   0},
	{169,   1},  {170,   1},
	{  0,   0},  {171,   1},
	{  0,   0},  {172,   1},
	{173,   1},  {  0,   0},
	{174,   1},  {  0,   0},
	{175,   1},  {176,   1},
	{  0,   0},  {177,   1},
	{  0,   0},  {178,   1},
	{179,   1},  {  0,   0},
	{180,   1},  {  0,   0},
	{181,   1},  {182,   1},
	{  0,   0},  {183,   1},
	{  0,   0},  {184,   1},
	{  0,   0},  {185,   1},
	{186,   1},  {  0,   0},
	{187,   1},  {  0,   0},
	{188,   1},  {  0,   0},
	{189,   1},  {  0,   0},
	{190,   1},  {191,   1},
	{  0,   0},  {192,   1},
	{  0,   0},  {193,   1},
	{  0,   0},  {194,   1},
	{  0,   0},  {195,   1},
	{  0,   0},  {196,   1},
	{  0,   0},  {197,   1},
	{198,   1},  {  0,   0},
	{199,   1},  {  0,   0},
	{200,   1},  {  0,   0},
	{201,   1},  {  0,   0},
	{202,   1},  {  0,   0},
	{203,   1},  {  0,   0},
	{204,   1},  {  0,   0},
	{205,   1},  {  0,   0},
	{206,   1},  {  0,   0},
	{207,   1},  {  0,   0},
	{208,   1},  {  0,   0},
	{209,   1},  {  0,   0},
	{210,   1},  {  0,   0},
	{211,   1},  {  0,   0},
	{212,   1},  {  0,   0},
	{213,   1},  {  0,   0},
	{  0,   0},  {214,   1},
	{  0,   0},  {215,   1},
	{  0,   0},  {216,   1},
	{  0,   0},  {217,   1},
	{  0,   0},  {218,   1},
	{  0,   0},  {219,   1},
	{  0,   0},  {220,   1},
	{  0,   0},  {221,   1},
	{  0,   0},  {  0,   0},
	{222,   1},  {  0,   0},
	{223,   1},  {  0,   0},
	{224,   1},  {  0,   0},
	{225,   1},  {  0,   0},
	{  0,   0},  {226,   1},
	{  0,   0},  {227,   1},
	{  0,   0},  {228,   1},
	{  0,   0},  {229,   1},
	{  0,   0},  {  0,   0},
	{230,   1},  {  0,   0},
	{231,   1},  {  0,   0},
	{232,   1},  {  0,   0},
	{233,   1},  {  0,   0},
	{  0,   0},  {234,   1},
	{  0,   0},  {235,   1},
	{  0,   0},  {  0,   0},
	{236,   1},  {  0,   0},
	{237,   1},  {  0,   0},
	{238,   1},  {  0,   0},
	{  0,   0},  {239,   1},
	{  0,   0},  {240,   1},
	{  0,   0},  {241,   1},
	{  0,   0},  {  0,   0},
	{242,   1},  {  0,   0},
	{243,   1},  {  0,   0},
	{  0,   0},  {244,   1},
	{  0,   0},  {245,   1},
	{  0,   0},  {  0,   0},
	{246,   1},  {  0,   0},
	{247,   1},  {  0,   0},
	{  0,   0},  {248,   1},
	{  0,   0},  {249,   1},
	{  0,   0},  {  0,   0},
	{250,   1},  {  0,   0},
	{251,   1},  {  0,   0},
	{  0,   0},  {252,   1},
	{  0,   0},  {253,   1},
	{  0,   0},  {  0,   0},
	{254,   1},  {  0,   0},
	{  0,   0},  {255,   1},
};

#endif

