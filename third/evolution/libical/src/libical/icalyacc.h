#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
	float v_float;
	int   v_int;
	char* v_string;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	DIGITS	257
# define	INTNUMBER	258
# define	FLOATNUMBER	259
# define	STRING	260
# define	EOL	261
# define	EQUALS	262
# define	CHARACTER	263
# define	COLON	264
# define	COMMA	265
# define	SEMICOLON	266
# define	MINUS	267
# define	TIMESEPERATOR	268
# define	TRUE	269
# define	FALSE	270
# define	FREQ	271
# define	BYDAY	272
# define	BYHOUR	273
# define	BYMINUTE	274
# define	BYMONTH	275
# define	BYMONTHDAY	276
# define	BYSECOND	277
# define	BYSETPOS	278
# define	BYWEEKNO	279
# define	BYYEARDAY	280
# define	DAILY	281
# define	MINUTELY	282
# define	MONTHLY	283
# define	SECONDLY	284
# define	WEEKLY	285
# define	HOURLY	286
# define	YEARLY	287
# define	INTERVAL	288
# define	COUNT	289
# define	UNTIL	290
# define	WKST	291
# define	MO	292
# define	SA	293
# define	SU	294
# define	TU	295
# define	WE	296
# define	TH	297
# define	FR	298
# define	BIT8	299
# define	ACCEPTED	300
# define	ADD	301
# define	AUDIO	302
# define	BASE64	303
# define	BINARY	304
# define	BOOLEAN	305
# define	BUSY	306
# define	BUSYTENTATIVE	307
# define	BUSYUNAVAILABLE	308
# define	CALADDRESS	309
# define	CANCEL	310
# define	CANCELLED	311
# define	CHAIR	312
# define	CHILD	313
# define	COMPLETED	314
# define	CONFIDENTIAL	315
# define	CONFIRMED	316
# define	COUNTER	317
# define	DATE	318
# define	DATETIME	319
# define	DECLINECOUNTER	320
# define	DECLINED	321
# define	DELEGATED	322
# define	DISPLAY	323
# define	DRAFT	324
# define	DURATION	325
# define	EMAIL	326
# define	END	327
# define	FINAL	328
# define	FLOAT	329
# define	FREE	330
# define	GREGORIAN	331
# define	GROUP	332
# define	INDIVIDUAL	333
# define	INPROCESS	334
# define	INTEGER	335
# define	NEEDSACTION	336
# define	NONPARTICIPANT	337
# define	OPAQUE	338
# define	OPTPARTICIPANT	339
# define	PARENT	340
# define	PERIOD	341
# define	PRIVATE	342
# define	PROCEDURE	343
# define	PUBLIC	344
# define	PUBLISH	345
# define	RECUR	346
# define	REFRESH	347
# define	REPLY	348
# define	REQPARTICIPANT	349
# define	REQUEST	350
# define	RESOURCE	351
# define	ROOM	352
# define	SIBLING	353
# define	START	354
# define	TENTATIVE	355
# define	TEXT	356
# define	THISANDFUTURE	357
# define	THISANDPRIOR	358
# define	TIME	359
# define	TRANSPAENT	360
# define	UNKNOWN	361
# define	UTCOFFSET	362
# define	XNAME	363
# define	ALTREP	364
# define	CN	365
# define	CUTYPE	366
# define	DAYLIGHT	367
# define	DIR	368
# define	ENCODING	369
# define	EVENT	370
# define	FBTYPE	371
# define	FMTTYPE	372
# define	LANGUAGE	373
# define	MEMBER	374
# define	PARTSTAT	375
# define	RANGE	376
# define	RELATED	377
# define	RELTYPE	378
# define	ROLE	379
# define	RSVP	380
# define	SENTBY	381
# define	STANDARD	382
# define	URI	383
# define	TIME_CHAR	384
# define	UTC_CHAR	385


extern YYSTYPE ical_yylval;

#endif /* not BISON_Y_TAB_H */
