/* ee-logs.h */
/*
 In EEPROM logger

 Copyright (C) 2010,2019 Potrepalov I.S.  potrepalov@list.ru

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
	Create logs in EEPROM memory.

	A log is a ring of records in EEPROM.

	The most significant bit in the last byte of record is used
	for service.

	Here we think that log is full (all records in ring filled with
	right info).


 Used extern functions:

 unsigned char ReadEE( void * ADDR )
	read one byte from EEPROM at address ADDR

 unsigned char isEEfree( void )
	return not 0, if EEPROM is free now;
	return 0, if EEPROM is busy

 void WriteEE( void * ADDR, unsigned char BT )
	write one byte BT to EEPROM at address ADDR
	return immediate (not wait for finish writing)


 Define these macros and functions:

 DECLARE_LOGGER( NAME, RECS, REC_SIZE, START_ADDR )
	declare log with name NAME, number of records RECS,
	size of record REC_SIZE in memory started at START_ADDR

	must have:
		2 <= REC_SIZE <= 255
		2 <= RECS <= 255
	one can use RECS-1 records (one record may be corrupted
	and it never read)

 LOGGER( NAME, RECS, REC_SIZE, START_ADDR )
	define log

	One can declare log many times, but define only one time.

	Before work with a log one must initialize it with function Log_Init


 Log_Init( NAME )
	initialize log with name NAME;
	the first record of the log become the 'current record'

 Log_ReadFirst( NAME, void * DST )
	read the first record of log NAME to address DST
	(buffer at address DST must have REC_SIZE bytes);
	the first record of the log become the 'current record'

 Log_ReadLast( NAME, void * DST )
	read the last record of log NAME to address DST
	(buffer at address DST must have REC_SIZE bytes);
	the last record of the log become the 'current record'

 Log_ReadNext( NAME, void * DST )
	read the next record of log NAME to address DST
	(buffer at address DST must have REC_SIZE bytes);
	if the 'current record' is the last record of the log
	then return 0 and no read anything;
	else the read record become the 'current record' and
	return not 0

 Log_ReadPrev( NAME, void * DST )
	read the previous record of log NAME to adderss DST
	(buffer at address DST must have REC_SIZE bytes);
	if the 'current record' is the first record of the log
	then return 0 and no read anything;
	else the read record become the 'current record' and
	return not 0

 Log_NoblockingWrite( NAME, void * SRC )
	no blocking append record to log NAME from address SRC;
	the record is appended after the last record of the log
	(on place of the first record of the log; second record
	of the log become the first record, third record become second
	and so on; appended record become the last record of the log);
	return not 0 if writing is started;
	return 0 while writing is in progress;
	to check state call this function with SRC = 0
	(if returned not 0 then writing is terminated);
	for real writing data to EEPROM call this function
	periodical with SRC = 0

 Log_ReadCur( NAME, void * DST )
	read the 'current record' of log NAME to adderss DST
	(buffer at address DST must have REC_SIZE bytes);

*/


#define LOG_FLAG_MASK	((unsigned char)0x80)


#define DECLARE_LOGGER( name, recs, rec_size, start_addr )		\
									\
void Log_InitLog ## name ( void );					\
void Log_ReadFirst ## name ( unsigned char * dst );			\
void Log_ReadLast ## name ( unsigned char * dst );			\
unsigned char Log_ReadNext ## name ( unsigned char * dst );		\
unsigned char Log_ReadPrev ## name ( unsigned char * dst );		\
unsigned char Log_NoblockingWrite ## name ( const unsigned char * src );\
									\
static inline void							\
Log_ReadCur ## name ( unsigned char * dst )				\
{									\
	extern unsigned char Log_CurReadRec__ ## name;			\
	void Log_ReadRec__ ## name ( unsigned char *, unsigned char );	\
	Log_ReadRec__ ## name ( dst, Log_CurReadRec__ ## name );	\
}


#define Log_Init( name )		Log_InitLog ## name ()
#define Log_ReadFirst( name, dst )	Log_ReadFirst ## name ( dst )
#define Log_ReadLast( name, dst )	Log_ReadLast ## name ( dst )
#define Log_ReadNext( name, dst )	Log_ReadNext ## name ( dst )
#define Log_ReadPrev( name, dst )	Log_ReadPrev ## name ( dst )
#define Log_NoblockingWrite( name, src )	\
					Log_NoblockingWrite ## name ( src )
#define Log_ReadCur( name, dsc )	Log_ReadCur ## name ( dst )


/* ------------------------------------------------------------------- */

#define Log_ReadFlag( addr )  ( LOG_FLAG_MASK & ReadEE((void*)(addr)) )



#define LOGGER( name, recs, rec_size, start_addr )			\
									\
static unsigned char Log_RecBuf__ ## name [rec_size];			\
									\
static unsigned char Log_CurRec__ ## name;				\
static unsigned char Log_CurFlag__ ## name;				\
									\
unsigned char Log_CurReadRec__ ## name;	/* 'current record' */ 		\
									\
void									\
Log_ReadRec__ ## name ( unsigned char * dst, unsigned char r )		\
{									\
	unsigned char i = (rec_size)-1;					\
	unsigned int a = (unsigned int)(start_addr) + r * (rec_size);	\
	do {								\
		*dst++ = ReadEE( (void*) a );		 		\
		++a;							\
	} while ( --i );						\
	*dst = (unsigned char)~LOG_FLAG_MASK & ReadEE( (void*) a );	\
}									\
									\
void									\
Log_InitLog ## name ( void )						\
{									\
	unsigned int a = (unsigned int)(start_addr) + (rec_size);	\
	unsigned char f = Log_ReadFlag( a );				\
	unsigned char cr = 1;						\
	Log_CurFlag__ ## name = f;					\
	do {								\
		a += rec_size;						\
		if ( (unsigned char)(f ^ Log_ReadFlag(a)) )		\
		{							\
			Log_CurRec__ ## name = cr;			\
			if ( cr -= (recs)-1 ) cr += (recs);		\
			Log_CurReadRec__ ## name = cr;			\
			return;						\
		}							\
		++ cr;							\
	} while ( cr < (recs) );					\
	Log_CurRec__ ## name = 0;					\
	Log_CurReadRec__ ## name = 1;					\
	Log_CurFlag__ ## name = f ^ LOG_FLAG_MASK;			\
}									\
									\
void									\
Log_ReadFirst ## name ( unsigned char * dst )				\
{									\
	unsigned char r = Log_CurRec__ ## name;				\
	if ( r -= (recs)-1 ) r += (recs);				\
	Log_CurReadRec__ ## name = r;					\
	Log_ReadRec__ ## name ( dst, r );				\
}									\
									\
void									\
Log_ReadLast ## name ( unsigned char * dst )				\
{									\
	unsigned char r = Log_CurRec__ ## name;				\
	if ( r ) -- r;							\
	else r = (recs) - 1;						\
	Log_CurReadRec__ ## name = r;					\
	Log_ReadRec__ ## name ( dst, r );				\
}									\
									\
unsigned char								\
Log_ReadNext ## name ( unsigned char * dst )				\
{									\
	unsigned char r = Log_CurReadRec__ ## name;			\
	if ( r -= (recs)-1 ) r += (recs);				\
	if ( r == Log_CurRec__ ## name ) return 0;			\
	Log_CurReadRec__ ## name = r;					\
	Log_ReadRec__ ## name ( dst, r );				\
	return 1;							\
}									\
									\
unsigned char								\
Log_ReadPrev ## name ( unsigned char * dst )				\
{									\
	unsigned char r = Log_CurReadRec__ ## name;			\
	if ( r ) -- r;							\
	else r = (recs) - 1;						\
	if ( r == Log_CurRec__ ## name ) return 0;			\
	Log_CurReadRec__ ## name = r;					\
	Log_ReadRec__ ## name ( dst, r );				\
	return 1;							\
}									\
									\
unsigned char								\
Log_NoblockingWrite ## name ( const unsigned char * src )		\
{									\
	static unsigned int a = 0;					\
	static unsigned char i;						\
	unsigned char * p;						\
	if ( !isEEfree() ) return 0;					\
	if ( a )							\
	{								\
		if ( i == (rec_size) ) { a = 0; goto test; }		\
		if ( i == (rec_size)-1 )				\
		{ /* write the last byte of record */			\
			unsigned char r;				\
			r = Log_RecBuf__ ## name [i];			\
			r &= (unsigned char)~LOG_FLAG_MASK;		\
			WriteEE( (void*) a, Log_CurFlag__ ## name | r );\
			r = Log_CurRec__ ## name;			\
			if ( r -= (recs)-1 ) r += (recs);		\
			else Log_CurFlag__ ## name ^= LOG_FLAG_MASK;	\
			if ( (i = Log_CurReadRec__ ## name) == r )	\
				Log_CurReadRec__ ## name =		\
					(i == (recs)-1) ? 0 : i + 1;	\
			Log_CurRec__ ## name = r;			\
			i = (rec_size);					\
		} else {						\
			WriteEE( (void*) a, Log_RecBuf__ ## name [i] );	\
			++a; ++i;					\
		}							\
		return 0;						\
	}								\
test:	if ( !src ) return 1;						\
	a = (unsigned int)(start_addr)					\
		+ (rec_size) * Log_CurRec__ ## name;			\
	i = (rec_size)-1;						\
	p = Log_RecBuf__ ## name;					\
	WriteEE( (void*) a, *src++ );					\
	++a;								\
	*p++ = 1; /* Log_RecBuf__ ## name[0] = 1 */			\
	do {								\
		*p++ = *src++ ;						\
	} while ( --i );						\
	i = 1;								\
	return 1;							\
}


/* End of file  ee-logs.h */
