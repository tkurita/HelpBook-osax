#include <Carbon/Carbon.r>

#define Reserved8   reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved
#define Reserved12  Reserved8, reserved, reserved, reserved, reserved
#define Reserved13  Reserved12, reserved
#define dp_none__   noParams, "", directParamOptional, singleItem, notEnumerated, Reserved13
#define reply_none__   noReply, "", replyOptional, singleItem, notEnumerated, Reserved13
#define synonym_verb__ reply_none__, dp_none__, { }
#define plural__    "", {"", kAESpecialClassProperties, cType, "", reserved, singleItem, notEnumerated, readOnly, Reserved8, noApostrophe, notFeminine, notMasculine, plural}, {}

resource 'aete' (0, "HelpBook Terminology") {
	0x1,  // major version
	0x0,  // minor version
	english,
	roman,
	{
		"HelpBook Suite",
		"Treat Help Books in bundles",
		'HBsu',
		1,
		1,
		{
			/* Events */

			"register helpbook",
			"Register a help book in specified bundle.",
			'HBsu', 'rgHB',
			'TEXT',
			"The name of the registered help book.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'file',
			"A reference to a bundle which contains a help book.",
			directParamOptional,
			singleItem, notEnumerated, Reserved13,
			{
				"recovering InfoPlist", 'rcIP', 'bool',
				"Recover Info.plist from recover-Info.plist in Resources folder, if required.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"show helpbook",
			"Show a help book in specfied bundle with Help Viewer.",
			'HBsu', 'shHB',
			'TEXT',
			"The name of the registered help book.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'file',
			"A reference to a bundle which contains a help book.",
			directParamOptional,
			singleItem, notEnumerated, Reserved13,
			{
				"recovering InfoPlist", 'rcIP', 'bool',
				"Recover Info.plist from recover-Info.plist in Resources folder, if ewquired.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"HelpBookOSAX version",
			"Version string of HelpBook.osax",
			'HBsu', 'Vers',
			'TEXT',
			"",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{

			}
		},
		{
			/* Classes */

		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
		}
	}
};
