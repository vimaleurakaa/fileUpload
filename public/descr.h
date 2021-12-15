/** 
 * @file Doxygen-style
 * @brief Описание функиций и методов TDescription и их возвращаемые значения (никак не описаны блоки try-catch)
 * @date 21 Октября 2021 (21.10.2021)
*/

#ifndef descrH
#define descrH

#include <stdlib.h>
#include <vector>
#include <map>

#include <algorithm>
#include <iostream>
#include <cctype>

using namespace std;

// avz - тянуть чтоли a101defs.h?
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#ifdef MY_SQL
	#define __LCC__
	#include <mysql/mysql.h>
#endif

#ifdef PG_SQL
    #include <QString>
    #include <QtSql/QtSql>
#endif

//#define DESCR_SERIALIZE   BOOST_SERIALIZATION_NVP

//! @enum Типы полей Descr-а
enum EDescrFieldType
{
	EFT_CHAR,
	EFT_UCHAR,
	EFT_SHORT,
	EFT_USHORT,
	EFT_INT,
	EFT_UINT,
	EFT_LONG,
	EFT_ULONG,
	EFT_FLOAT,
	EFT_DOUBLE,
	EFT_ENUM,
	EFT_STRUCT,
	EFT_CHAR_STRING,
	EFT_BOOL,
	EFT_BITFIELD,
	EFT_TIME,
	EFT_INVALID
};

//! @brief Вернуть enum с типом поля Descr-а на основе `typeid(ti)`
EDescrFieldType typeinfo2enum(const type_info &ti);

typedef  long long tagIDFunc(char*);
typedef  tagIDFunc  *IDFuncPtr;
typedef  map<string,vector<char*>*> DQueryMap;
typedef  map<string, vector<string>> DSQueryMap;
typedef  vector<int> NestedIndex; //!< Список вложенных индексов (индексы родит. эл-ов + текущего поля)

/**
 * @brief Класс для работы с метаинформацией о перечислимом типе
 * @note Класс хранит значения не больше `ulong`.
 */
struct TEnumDescription
{
	// Внутренние и вспомогательные методы
	/**
	 * @brief Получение метаинформации о значении перечислимого типа
	 *
	 * @param[in] ix Искомый индекс значения, или 0 для получения количества значений, или -1 для поиска по значению
	 * @param[in] val Искомое значение. Используется, если `ix` == -1
	 * @param[out] pNumVal Указатель для возврата числового значения
	 * @param[out] szShortStr Указатель для возврата сокращенного строкового значения
	 * @param[out] szFullStr Указатель для возврата полного строкового значения
	 *
	 * @return Признак успеха или количество значений:
	 *		* `> 0` --- Количество значений
	 *		* `0` --- Поле получено успешно
	 *		* `-1` --- Ошибка
	 */
	virtual int getEntry(int ix, ulong val, ulong *pNumVal = nullptr, char **szShortStr = nullptr, char **szFullStr = nullptr) = 0;
	/**
	 * @brief Получение указателя на значение
	 * @return Указатель на значение
	 */
	virtual void *getBase() = 0;

	// Информация о перечислимом типе
	//! @brief Имя типа
	virtual char *getName() = 0;
	//! @brief Дополнительное текстовое описание типа
	virtual const char *getComment() = 0;
	//! @brief Размер enum'а (sizeof)
	virtual size_t getSize() = 0;
	//! @brief Возвращает тип поля Descr-а о значении перечислимого типа (по-умолчанию `uchar`-а) @see EDescrFieldType
	virtual EDescrFieldType getType() = 0;
	//! @brief Минимальное корректное значение
	ulong getMinValue() { return getValueByIndex(1); }
	//! @brief Максимальное корректное значение
	ulong getMaxValue() { return getValueByIndex(getEntryCount()); }

	// Доступ перебором
	//! @brief Общее количество значений
	ulong getEntryCount() { return getEntry(0, 0); }
	//! @brief Числовое значение по заданному индексу в списке значений
	ulong getValueByIndex(size_t ix) { ulong res = 0; return (getEntry(ix, 0, &res) == 0) ? res : 0; }
	//! @brief Все параметры значения по заданному индексу в списке значений
	bool getEntryByIndex(int ix, ulong *pNumVal = nullptr, char **szShortStr = nullptr, char **szFullStr = nullptr)
	{
		if (ix < 0) return false;	return (getEntry(ix, 0, pNumVal, szShortStr, szFullStr) == 0);
	}
	//! @brief Полное строковое значение по заданному индексу в списке значений
	char *Index2Str(size_t ix) { char *res = nullptr; return (getEntry(ix, 0, nullptr, nullptr, &res) == 0) ? res : nullptr; }
	//! @brief Получить сокращенное строковое значение по заданному индексу в списке значений
	char *Index2ShortStr(size_t ix) { char *res = nullptr; return (getEntry(ix, 0, nullptr, &res) == 0) ? res : nullptr; }

	// Доступ по заданному значению
	//! @brief Полную информацию по заданному значению
	bool getEntryByValue(ulong val, ulong *pNumVal = nullptr, char **szShortStr = nullptr, char **szFullStr = nullptr)
	{
		return (getEntry(-1, val, pNumVal, szShortStr, szFullStr) == 0);
	}
	//! @brief Полное строковое значение по заданному числовому значению
	char *Enum2Str(ulong val) { char *ptr = nullptr;  return (getEntryByValue(val, nullptr, nullptr, &ptr)) ? ptr : nullptr; }
	//! @brief Сокращенное строковое значение по заданному числовому значению
	char *Enum2ShortStr(ulong val) { char *ptr = nullptr;  return (getEntryByValue(val, nullptr, &ptr)) ? ptr : nullptr; }


	// Информация о конкретном сохраненном значении
	//! @brief Числовое значение элемента 
	//! @note указанного через `D_ENUMVAL` или `D_ENUMVALUE`
	virtual ulong getValue(size_t iArrayShift = 0) = 0;
	//! @brief Строковое значение 
	//! @note указанного через `D_ENUMVAL` или `D_ENUMVALUE`
	char *getStr(size_t iArrayShift = 0) { return Enum2Str(getValue(iArrayShift)); }
	//! @brief Сокращенное строковое значение 
	//! @note указанного через `D_ENUMVAL` или `D_ENUMVALUE`
	virtual char *getShortStr(size_t iArrayShift = 0) { return Enum2ShortStr(getValue(iArrayShift)); }
	//! @brief Корректность значения (должно быть >= `0`)
	//! @note указанного через `D_ENUMVAL` или `D_ENUMVALUE`
	virtual bool getValid(size_t iArrayShift = 0) { return (getEntry(-1, getValue(iArrayShift)) >= 0); }
};

struct TDescription;

//! Прикладной тип данных (физическая величина + единицы измерения)
enum EDescrLogicType
{
	ELT_INVALID				= 0,			//!< не определен

	// простые единицы измерения
	ELT_SEC					= 0x0101,		//! секунды
	ELT_MILLISEC			= 0x0102,		//! миллисекунды

	ELT_METER				= 0x0110,		//!< метры
	ELT_KILOMETER			= 0x0111,		//!< километры

	ELT_RADIAN				= 0x0120,		//!< радианы
	ELT_DEGREE				= 0x0121,		//!< градусы

	ELT_KILOGRAM			= 0x0130,		//!< килограммы
	ELT_CENTNER				= 0x0131,		//!< центнеры
	ELT_TON					= 0x0132,		//!< тонны

	ELT_PERCENT				= 0x1000,		//!< проценты (сотая часть чего-либо)

	// составные типы (комбинация простых)
	ELT_METER_SEC			= 0x0211,		//!< метры в секунду
	ELT_KILOMETER_HOUR		= 0x0212,		//!< километры в час

	ELT_RADIAN_SEC			= 0x0221,		//!< радиан в секунду
	ELT_DEGREE_SEC			= 0x0222,		//!< градусы в секунду

	ELT_MEASUREMENT_UNITS	= 0x7FFF,		//!< маска для извлечения единиц измерения
	//------------------------------------------------------------------------------

	// перечислимый тип
	ELT_ENUM				= 0x00008000,	

	// семантика поля
	ELT_ABSOLUTE_DATE		= 0x00110000,	//!< дата (относительно 01.01.1970)
	ELT_RELATIVE_DATE		= 0x00120000,	//!< интервал дат
	ELT_ABSOLUTE_DATETIME	= 0x00130000,	//!< дата и время (относительно 01.01.1970 0:00:00)
	ELT_RELATIVE_DATETIME	= 0x00140000,	//!< интервал с указанием даты и времени
	ELT_RELATIVE_TIME		= 0x00150000,	//!< относительное время

	ELT_GEOCENTRIC_XYZ		= 0x00210000,	//!< координата в геоцентрической системе
	ELT_GEOGRAPHIC_F		= 0x00220000,	//!< географическая (геодезическая) широта
	ELT_GEOGRAPHIC_L		= 0x00230000,	//!< географическая (геодезическая) долгота
	ELT_UT_XY				= 0x00240000,	//!< координата в системе УТ
	ELT_POLAR_AZIM			= 0x00250000,	//!< азимут в полярных координатах
	ELT_POLAR_DIST			= 0x00260000,	//!< дальность в полярных координатах
	ELT_HEIGHT				= 0x00270000,	//!< высота

	ELT_COURSE				= 0x00310000,	//!< курс
	ELT_VELOCITY			= 0x00320000,	//!< скорость
	ELT_FUEL				= 0x00330000,	//!< топливо
	ELT_DISTANCE			= 0x00340000,	//!< расстояние
	ELT_ANGLE				= 0x00350000,	//!< угол

	ELT_SEMANTIC_TYPE		= 0xFFFF0000	//!< маска для извлечения семантики
	//-----------------------------------------------------------------------
};

//! Формат вывода данных
enum EDescrCompositeFormat
{
	ECF_INVALID,		//!< не определен

	// enum'ы
	ECF_ENUM_RAW,		// в исходном виде
	ECF_ENUM_SHORT_STR, // краткая строка
	ECF_ENUM_STR,		// полная строка

	// время
	ECF_DATE,			//!< текст в формате "dd.mm.yyyy"
	ECF_DATE_TIME,		//!< текст в формате "dd.mm.yyyy hh:mm:ss"
	ECF_HOUR_MIN_SEC_T,	//!< текст в формате "hh:mm:ss.ttt"
	ECF_HOUR_MIN_SEC,	//!< текст в формате "hh:mm:ss"
	ECF_MIN_SEC_T,		//!< текст в формате "mm:ss.ttt"
	ECF_MIN_SEC,		//!< текст в формате "mm:ss"
	ECF_HOUR_MIN,		//!< текст в формате "hh:mm"
	ECF_HOUR,			//!< текст в формате "hh"
	ECF_MIN,			//!< текст в формате "mm"

	// географические координаты
	ECF_FL_GRAD_MIN_SEC,		//!< gg/mm/ss
	ECF_FL_FRACTION,			//!< gg.ttt
	
};

//! @brief Структура с метаинформацией о конкретном поле/массиве 
//! @remark Доступ к вложенным структурам только на время ее жизни
struct DFieldInfo
{
	EDescrFieldType eFT; //!< Тип поля
	void *pVA;	//!< Значение
	int iSize;
	char *szName;
	bool isArray;
	int logicType; // EDescrLogicType
	void *minval;
	void *maxval;
	void *vals;
	bool invalidVals;

	DFieldInfo() { memset(this, 0, sizeof(*this)); }
	~DFieldInfo() { if (pD) delete pD; if (pE) delete pE; }

	/**
	 * @brief Получить форматированное значение параметра поля Дескрипшена
	 * @param eLT[in]	Тип прикладных данных поля
	 * @param eCF[in]	Формат ввода данных
	 * @param precision[in]	 Точность после запятой
	 * @param digits[in]	Кол-во заполнителей 0-ми (прим. digits=4, value=55 => ответ=0055)
	 */
	std::string getFormatStringVal(EDescrLogicType eLT, EDescrCompositeFormat eCF, int precision, int digits);

	void setPD(TDescription *ptr) { if (pD) delete pD; pD = ptr; }
	void setPE(TEnumDescription *ptr) { if (pE) delete pE; pE = ptr; }

	//! Получение доступа к указателям на время жизни структуры DFieldInfo
	TDescription *getPD() { return pD; }
	TEnumDescription *getPE() { return pE; }

	//! Извлечение указателя в неограниченное пользование (требует удаления объекта вручную)
	//! в структуре он при этом обнуляется
	TDescription *takePD() { TDescription *ptr = pD; pD = nullptr; return ptr; }
	TEnumDescription *takePE() { TEnumDescription *ptr = pE; pE = nullptr; return ptr; }

private:
	DFieldInfo(const DFieldInfo&) = delete; // копирование запрещено
	void operator=(const DFieldInfo&) = delete; // копирование запрещено

	// пересчет значения в нужный тип и формат, с заданной точностью
	template<typename T> std::string recalcToUnits(EDescrLogicType eLT, EDescrCompositeFormat eCF, int precision, int digits);

	// прямой доступ запрещен для автоудаления неиспользуемых объектов
	TDescription *pD;		//!< Указатель на Descr подструктуры/родителя
	TEnumDescription *pE;	//!< Указатель на Descr enum-а (для ENUMTYPE)
};


//! @brief Класс для работы с метаинформацией о структуре
struct TDescription
{
	TDescription();
	//! Конструктор @param name --- имя класса, который наследуем --- `DEPRECATED`
	TDescription(const char *name);
	virtual ~TDescription() {};

private:
	//! Имя структуры, которая унаследована от TDescription
	std::string  structName;                                             
	//! Рекурсивный поиск поля по вложенному индексу
	//static int getNestedFieldRec(TDescription *descr, size_t level, NestedIndex ix, NestedIndex shiftStack, EDescrFieldType &eFT, void **pVA, const char **szName, int *pSize);
	static int getNestedFieldRec(TDescription *descr, size_t level, NestedIndex ix, NestedIndex shiftStack, DFieldInfo &fi);

public:
#ifdef PG_SQL
	QSqlDatabase *db;
#endif

	////////////////////////////////////////////////////////////////////////////////////////////
	////							Вспомогательные методы									////
	////////////////////////////////////////////////////////////////////////////////////////////
	/**
	 * @brief Получение метаинформации поля по индексу
	 * @note(@kotborealis): Документация выведена чисто с помощью реверс-инжиниринга исходников,
	 *      инетпретация может отличаться от действительности и авторской задумки
	 *
	 * @param[in] ix 			Индекс поля
	 * @param[out] fi 			Информация о поле структуры
	 * @param[in] iArrayShift 	Сдвиг массива
	 * @remark Если @c ix == 0, то возвращает количество полей в не сплошной (не плоской) нумерации. 
	 * @remark Данный метод описывается в макросах, поэтому не ищите его в `desct.cpp`.
	 *
	 * @warning Для полей, описанных макросами `D_ENUMTYPE`, `D_STRUCT`, `D_STRUCT_MEM`, `D_STRUCT_CONST`, `D_BASETYPE`, в `fi.pD` **выделяется память** --- не забудте её удалить !
	 * @warning Для полей, описанных макросами `D_VALIDS`, `D_INVALIDS`, в `fi.vals` **выделяется память** --- не забудте её удалить !
	 * @warning Для полей, описанных макросами `D_MIN`, `D_MINMAXMAX`, в `fi.minval` **выделяется память** --- не забудте её удалить !
	 * @warning Для полей, описанных макросами `D_MAX`, `D_MINMAXMAX`, в `fi.maxval` **выделяется память** --- не забудте её удалить !
	 *
	 * @return Признак успеха:
	 * @return `0` --- поле получено успешно ИЛИ в Дескрипшене нет полей
	 * @return `-1` --- ошибка (выход за границы `ix < 0`, `ix >` _полей Дескрипшена_)
	 * @return `>0` --- количество полей в Дескрипшене (только если `ix == 0`)
	 */
	virtual int getField(size_t ix, DFieldInfo &fi, size_t iArrayShift) = 0;
	/**
	* @brief Записывает в `out` значения поля, в виде `prefixимя=значение \n`
	* * `значение` для времени --- `???`
	* * `значение` для массивов --- `prefixимя=[значение1,значение2,...]`
	* * `значение` для структур --- `prefixимя=( поле1=значение1,поле2=значение2,... )`
	*
	* @param[in] ix	Индекс поля
	* @param[out] out	Поток вывода
	* @param[in] prefix 	Префикс
	* @param[in] iArrayShift 	Сдвиг массива
	*
	* @see streamStruct
	* @return	Признак успеха:
	* @return	* `false` --- `ix <= 0` ИЛИ указанное поле имеет `iSize < 0`
	*/
	bool streamField(int ix, std::ostream &out, const char *prefix, size_t iArrayShift);
	/**
	 * @brief Записывает в @c out значения полей @c descr
	 * Вызовает streamField для каждого поля Дескрипшена
	 * @see streamField
	 */
	friend std::ostream& streamStruct(std::ostream &out, TDescription &descr, const char *prefix, size_t iArrayShift);
	//! Возвращает базовый элемент, на котором основан Descr
	virtual void *getBase()
	{
     	return 0;
	};
	virtual size_t getSize() = 0;

	////////////////////////////////////////////////////////////////////////////////////////////
	////									Интерфейс										////
	////////////////////////////////////////////////////////////////////////////////////////////
	//! Получить имя структуры --- `DEPRECATED`
	std::string getName() { return structName; }
	//! @brief Установить имя структуры
	void setName(std::string n) { structName = n; }
	//! @brief Дополнительное текстовое описание типа
	virtual const char *getComment() = 0;
	//! @brief Получить количество родных полей в структуре
	int  getFieldCount();
	/**
	 *	@brief Получить поле в виде строки `prefixимя[ArrayShift]=значение\n`
	 *	
	 *	@param[in] ix Номер поля
	 *	@param[out] dest Записанное поле, в виде строки (буфер)
	 *	@param[in] prefix Префикс
	 *	@param[in] iArrayShift Смещение
	 *	
	 *	@remark Не забудте её выделить память в dest __до__ и удалить __после__ метода.
	 *	@warning Если тип поле является значением или массивом, то этот буфер перезаписывается. А у структур это поле дополняется. Получается, что перед структурой `dest` надо очищать. _Звучит не шаблонненько_.
	 *	@note На момент 23.09.2021 смещение не работает. Всегда возвращает строку из всех элементов _массива_. Т.е. при вызове смещение происходит, но обрабатывается весь размер массива (выходит за пределы памяти).
	 *	
	 *	@return Признак успеха, @a true --- поле получено успешно.
	 */
	bool getFieldString(int ix, char *dest, char *prefix, size_t iArrayShift);
	/** 
	 * @brief Получить поле в виде строки `prefixимя[ArrayShift] = начало+размер`
	 * @note На момент 23.09.2021 не обрабатывает ложное смещение (т.е. там где не может быть смещения или отрицательное).
	 * @note На момент 23.09.2021 смещение не работает. Всегда возвращает строку из всех элементов _массива_. Т.е. при вызове смещение происходит, но обрабатывается весь размер массива (выходит за пределы памяти).
	 */
	bool getMetaFieldString(int ix, char *dest, char *prefix, size_t iArrayShift);
	//! Получить все поля структуры в виде массива `prefixимя[ArrayShift] = значение\n...`
	bool getDescription(char *dest, char *prefix, size_t iArrayShift = 0);
	//! @brief Получить все поля структуры в виде массива `prefixимя[ArrayShift] = начало+размер\n...`
	bool getMetaDescription(char *dest, char *prefix, size_t iArrayShift = 0);

	/**
	 * @brief Получить строковое значение параметра поля Дескрипшена
	 * @param val[out]	Возращает строковое значение
	 * @param fi[in]	Информация о поле
	 * @param isQuoteEnabled[in]	Обрамить `D_CHARSTRING` в "$pgdescr$" с обеих сторон
	 *
	 * @note На момент 18.10.2021 для буллевых значений возвращает значение с маленькой буквы (`true`/`false`);
	 * @note На момент 18.10.2021 тип `ulong` переводит в `long` (для записи в строку);
	 *
	 * @remark Для `val` необходимо заранее выделить и освободить память её после использования.
	 * @note Только для `EFT_CHAR`,`EFT_UCHAR`,`EFT_SHORT`,`EFT_USHORT`,`EFT_INT`,`EFT_UINT`,`EFT_LONG`,`EFT_ULONG`,`EFT_FLOAT`,`EFT_DOUBLE`,`EFT_ENUM`,`EFT_CHAR_STRING`,`EFT_BOOL`.
	 *
	 * @return `val`
	 */
	char* getStringVal(char * val, DFieldInfo &fi, bool isQuoteEnabled = false);
	/**
	 * @brief Получить значения параметра по индексу (в случае, если @c iSize > 1), т.е. getStringVal для массивов.
	 * @param val[out]	Возращает строковое значение
	 * @param fi[in]	Информация о поле
	 * @param idx[in]	Индекс
	 * @param isQuoteEnabled[in]	Обрамить `D_CHARSTRING` в "$pgdescr$" с обеих сторон
	 *
	 * @note На момент 18.10.2021 для буллевых значений возвращает значение с маленькой буквы (`true`/`false`);
	 * @note На момент 18.10.2021 тип `ulong` переводит в `long` (для записи в строку);
	 *
	 * @remark Для `val` необходимо заранее выделить и освободить память её после использования.
	 * @note Только для `EFT_CHAR`,`EFT_UCHAR`,`EFT_SHORT`,`EFT_USHORT`,`EFT_INT`,`EFT_UINT`,`EFT_LONG`,`EFT_ULONG`,`EFT_FLOAT`,`EFT_DOUBLE`,`EFT_ENUM`,`EFT_CHAR_STRING`,`EFT_BOOL`.
	 * @see getStringVal
	 *
	 * @return `val`
	 */
	char* getElemStringVal(char * val, DFieldInfo &fi, size_t idx, bool isQuoteEnabled = false);
	/**
	 * @brief Установить значение целочисленного поля
	 * @param val[in]	Новое значение
	 * @param fi[in]	Информация о поле
	 * @note На текущий момент (19.10.2021) значения устанавливаются только для `EFT_CHAR`,`EFT_UCHAR`,`EFT_SHORT`,`EFT_USHORT`,`EFT_INT`,`EFT_UINT`,`EFT_LONG`,`EFT_ULONG`,`EFT_FLOAT`,`EFT_DOUBLE`,`EFT_ENUM`.
	 * @note На момент 19.10.2021 для `EFT_CHAR_STRING`, `EFT_BOOL`, `EFT_BITFIELD` --- возвращает `false`.
	 * @note При этом важно, что хоть для `EFT_ENUM`, хоть для `EFT_DOUBLE` @c val типа `long long`.
	 * @return Признак успеха.
	 */
	bool setIntVal(long long val, DFieldInfo &fi);

	
	////////////////////////////////////////////////////////////////////////////////////////////
	////							Интерфейс по сплошной (плоской)							////
	////		Методы для перебора вложенных полей по сплошной (плоской) нумерации			////
	////	У них не может быть iArrayShift, т.к. массивы разворачиваются в отдельные поля	////
	////////////////////////////////////////////////////////////////////////////////////////////
	//! Получить количество полей в сплошной (плоской) нумерации (т.е. учитывая элементы в массивов)
	int  getFlatFieldCount();
	/**
	 * @brief Получить метаинформацию о поле в сплошной (плоской) нумерации 
	 * 
	 * @param ix[in] 	Номер поля (в плоской нумерации)
	 * @param fi[out]	Метаинформация о поле
	 * @param prependedname[in,out]	Буфер для полного имени поля, ДОЛЖЕН быть проинициализирован, МОЖЕТ содержать дополнительный префикс
	 * @param iArrayShift[in]	Смещение
	 * @remark Если @c ix == 0, то возвращает количество полей в сплошной (плоской) нумерации. 
	 * @remark Память в `fi.szName` контролируется пользователем:
	 * 	* Если при вызове `fi.szName` ссылается на `nullptr`, то под имя выделяется память. Не забудте её удалить!
	 * 	* Если при вызове `fi.szName` ссылается на выделенную память, то имя **дописывается** в поле в формате: `.nameField`. Именно с точкой.
	 * @warning Метод перебирает все поля до указанного ix, это значит, что если до этого ix есть STRUCT без полей или любая другая ошибка, то return будет 0 (или -1, соответственно).
	 *
	 * @todo Проверить факт: если последним полем стоит BITFIELD, то в `MSVC 2015` может возникнуть проблема --- значение `void* pVA` не совпадает со ссылкой на статическую глобальную переменную (аргумент D_BITFIELD).
	 * 
	 * @return `0` --- корректное заполнение всех аргументов 
	 * 		*	ИЛИ В Дескрипшене нет полей 
	 * 		*	ИЛИ есть STRUCT без полей (и она объявлена раньше, чем вызываемое поле)
	 * @return `-1` --- iSize<1 (у поля неправильно указан размер массива <1)
	 * 		* ИЛИ выход за границы (ix < 0), (ix > полей Дескрипшена)
	 * @return `>0` --- кол-во полей в плоской нумерации (только если ix == 0)
	 */
	int  getFlatField(int ix, DFieldInfo &fi, char *prependedname = nullptr, int internal = 0);
	/**
	 * @brief Получить значение поля в виде строки `имя=значение` в сплошной (плоской) нумерации 
	 * Если тип `ENUMTYPE` вид строки будет следующим `имя=числ_значение(строковое_значение)`
	 * @param ix[in]	Номер поля (в плоской нумерации)
	 * @param dest[in,out]	Дописывает записанную строку 
	 * @remark Не воспринимается тип `CHARSTRING` с размером больше 100000 знаков.
	 * @remark Eсли dest не пуст, то сначала ставится '.', а затем пишется значение поля.
	 * 
	 * @return Признак успеха
	*/
	bool getFlatFieldString(int ix, char *dest);

	// Интерфейс - методы для работы с вложенными индексами (индексы родит. эл-ов + текущего поля)
	/**
	 * @brief Получить список полных имен полей Descr-а через '.'
	 * @param names[out]	Список имён вложенных полей
	 * @param types[out]	Список типов вложенных полей
	 * @param nindexes[out]	Список вложенных индексов (т.е. индексов родительских элементов + поля (последний эл-т))
	 * @param parent[in]	Для рекурсивного указания родителя Descr-а
	 * @param prefix[in]	Префикс
	 * @remark Новые значения добавляются в конец списков.
	 */
	void getNestedFieldsInfo(vector<string> &names, vector<EDescrFieldType> &types, vector<NestedIndex> &nindexes, NestedIndex parent = NestedIndex(), string prefix = "");
	/**
	 * @brief Получить полное имя через '.' по вложенному индексу
	 * @note После вызова getNestedFieldName срабатывает деструктор, вызвавшего его Descr-а.
	 * @return Полное имя или `nullptr` при неправильном вл. индексе
	 */
	char* getNestedFieldName(NestedIndex &nindex);
	/**
	 * @brief Получить вложенный индекс по полному имени через '.'
	 * @return Вложенный индекс поля или `NestedIndex()` при неправильном полного пути.
	 */
	NestedIndex getNestedFieldIndex(const char *fullname);
	/**
	 * @brief Получить метаинформацию о поле по вложенному индексу и сдвигам в массивах
	 * @param ix[in]	Вложенный индекс
	 * @param shiftStack[in]	Сдвиг вложенного индекса ???
	 * @param fi[out]	Метаинформация о поле
	 * @return Признак успеха:
	 * @return `>0` --- Всё хорошло
	 * @return `-1` --- Не правильный вложенный индекс
	 * @see getNestedFieldRec (private)
	 */
	int  getNestedField(NestedIndex ix, NestedIndex shiftStack, DFieldInfo &fi);
	/**
	 * @brief Оператор потокового вывода значений полей структуры
	 * @note На момент 18.10.2021 массив буллов выводит значения [0,1], а не [True, False]
	 * @return Возвращает использованный поток вывода @c out
	 */
	friend std::ostream& operator<< (std::ostream &out, TDescription &descr);

	// Интерфейс - методы для проверки корректности
	//! Проверка собственной структуры
	bool check();
	//! Проверка подструктуры или другой структуры
	bool checkStruct(TDescription *descr, size_t iArrayShift = 0, const char *prefix = "");
	//! Проверка поля собственной структуры
	bool checkField(int ix, size_t iArrayShift, const char *prefix);
	//! Проверка ограничений для числовых типов
	template<typename T> bool checkNumbers(DFieldInfo &fi, size_t iArrayShift, const char *prefix);
	//! Проверка ограничений для строк
	bool checkStrings(DFieldInfo &fi, const char *prefix);

	// Интерфейс - методы для работы с БД PostgreSQL
#ifdef PG_SQL
	bool pgCreateTable(vector<char*> &queries);
	char *pgGetId();
    bool pgInsert(DQueryMap &queries, IDFuncPtr fptr, const char *nested_type=NULL, long long nested_id = 0L, const char *nested_field=NULL, int nested_num = 0);
    template<class IDFunc>
    bool pgCopy(DSQueryMap &qmap, IDFunc &&f, const char *nested_type = "", long long nested_id = 0L, const char *nested_field = "", int nested_num = 0, int array_size = 1);
    std::string pgTableName() {return "auto_" + this->strTolower(getName());}
#endif

	/**	
	 * @brief Получить XML-описания типа (Дескрипшена)
	 * 
	 * @warning 
	 * 	1. У XML-описания есть ограничение на 10240 байт. 
	 * 	2. Не забудте удалить возвращаемый массив.
	 *
	 * @return XML-описание всего Дескрипшена
	 */
	char *getXMLDescription();
	/**
	 * @brief Получить XML-описание поля
	 * @param[in] ix Индекс поля
	 * @param[out] dest Буфер для записи XML-описание
	 * 
	 * @return Признак успеха, `true` --- поле получено успешно.
	*/
	virtual bool getFieldTag(int ix, char *dest);

    #ifdef PG_SQL
		static std::map<pair<QString,Qt::HANDLE>, QSqlDatabase *> dbs; //!< Записывает существующие соединения
		/**
		 * @brief Создаёт соединение
		 * @param dbsection[in] Путь к БД
		 * @note По-умолчанию путь до БД `DBConnection`
		 * @return Соединение установлено
		*/
		static bool initDb(QString dbsection = "");
		static QSqlDatabase *getDBConnection(QString dbsection = "");
        bool querySelect(QString whereClause, size_t iArrayShift = 0, QString dbsection = "");
		static bool queryInt(QString query, int &iVal, QString dbsection = "");
		static bool pgSelectIntVector(QString query, vector<int> &vInts, QString dbsection); //unused
		static bool queryUpdate(QString query, QString dbsection = "");
		char *getIntKey(QString dbsection = "");
		bool update(QString dbsection, bool nested = false, char * extKey = NULL, char * extKeyValue = NULL, int arrayShift = 0);
		bool insert(QString dbsection, bool nested = false, char * extKey = NULL, char * extKeyValue = NULL, int arrayShift = 0);
		bool del(QString dbsection, bool nested, char *extKey, char *extKeyValue, int arrayShift);
		bool cleanAll(QString dbsection, bool nested = false); //!< очищает свою таблицу, таблицы вложенных типов структур и обнуляет счетчики идентификаторов!
	#endif

    static inline std::string strTolower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){return std::tolower(c);});

        return s;
    };
};

//! @todo Документировать
class DescriptionWalker {
public:
	DescriptionWalker(std::map<NestedIndex, int> &filter, std::vector<NestedIndex> &need);
	~DescriptionWalker();

	void setFilter(std::map<NestedIndex, int> &filter);
	void setNeeded(std::vector<NestedIndex> &indexes);

	std::map<NestedIndex, std::vector<double>> getValues(TDescription *root);

private:
	std::map<NestedIndex, int> filters;
	std::map<NestedIndex, int> arrayFilter;
	std::vector<NestedIndex> needed;
	NestedIndex longest;

	std::map<NestedIndex, std::vector<double>> retValues;

	std::vector<TDescription*> descrStack;
	NestedIndex indexStack;
	NestedIndex shiftStack;

	void buildStack(TDescription* td, int shift = 0, int level = 0);
	void checkStack();

};

bool isNestedFieldCompatible(TDescription *descr, NestedIndex existing, NestedIndex added);
bool isNestedFieldCompatible(NestedIndex existing, bool existingIsArray, NestedIndex added, bool addedIsArray);
void *getNestedValuePtr(vector<TDescription*> &descrstack, NestedIndex &indexstack, NestedIndex &shiftstack, NestedIndex &neededindex, EDescrFieldType &eFT, int &iSize);

//! Возвращает `base`, сдвинутый на `index` элементов типа `eFT`
void *shiftAddr(EDescrFieldType eFT, void *base, size_t index);
//! Возвращает `fi`, сдвинутый на `index` элементов типа `ENUMTYPE` (sizeof) 
void *shiftAddr(DFieldInfo &fi, size_t index);

//! @brief Вспомогательный класс для доступа к битовым полям
class DIBitAccessor
{
public:
	virtual int getval(void *obj) = 0;
	virtual void setval(void *obj, int val) = 0;
};

/******************************************************************************
@class DIBitAccessor descr.h

Пример определенного макросами класса DIBitAccessor:
@code{.cpp}
	//D_BITACCESSOR(DESTSTRUCT, DESTFIELD, CLASSNAME, OBJNAME) 
	class CLASSNAME: public DIBitAccessor 
	{ 
	public: 
		int getval(void *obj) { return ((DESTSTRUCT*)obj)->DESTFIELD; }
		void setval(void *obj, int val) { ((DESTSTRUCT*)obj)->DESTFIELD = val; }
	}; 
	CLASSNAME OBJNAME; 
@endcode
******************************************************************************/

std::ostream& operator<< (std::ostream &out, TDescription &descr);
std::ostream& operator<< (std::ostream &out, TDescription &&descr);
std::ostream& streamStruct(std::ostream &out, TDescription &descr, char *prefix, size_t iArrayShift);

#define D_BITACCESSOR(DESTSTRUCT, DESTFIELD, CLASSNAME, OBJNAME) class CLASSNAME: public DIBitAccessor { public: int getval(void *obj) {return ((DESTSTRUCT*)obj)->DESTFIELD;} void setval(void *obj, int val) {((DESTSTRUCT*)obj)->DESTFIELD = val;} }; static CLASSNAME OBJNAME; 


/*****************************************************************************/


#define DESCR_START(DBNAME, DBTYPE, DESCRTYPE, /* COMMENT (optional) */ ... ) struct DESCRTYPE: public TDescription { DBTYPE *base, *msg; void *getBase() {return reinterpret_cast<void*>(base);}; size_t getSize() {return sizeof(DBTYPE); }; const char *getComment() {return "" __VA_ARGS__;}; DESCRTYPE(DBTYPE *pMsg) { base = pMsg; this->setName(DBNAME); } \
    int getField(size_t ix, DFieldInfo &fi, size_t iArrayShift) { /*if (ix < 0) return -1;*/ size_t i = 0; fi.iSize=1; msg = base + iArrayShift; if (false) {

#define D_BOOL(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_BOOL; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef bool T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_CHAR(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_CHAR; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef char T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_UCHAR(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_UCHAR; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef uchar T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_SHORT(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_SHORT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef short T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_USHORT(FIELDNAME, QUOTEDFIELDNAME)	return 0; } if (ix == ++i) {fi.eFT=EFT_USHORT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef ushort T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_INT(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_INT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef int T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_UINT(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_UINT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef uint T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_LONG(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_LONG; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef long T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_ULONG(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_ULONG; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef ulong T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_FLOAT(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_FLOAT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef float T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_DOUBLE(FIELDNAME, QUOTEDFIELDNAME)	return 0; } if (ix == ++i) {fi.eFT=EFT_DOUBLE; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef double T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_ENUM(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_ENUM; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef int T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_ENUMTYPE(FIELDNAME, QUOTEDFIELDNAME, ENUMDESCR)  return 0; } if (ix == ++i) {fi.eFT=EFT_ENUM; fi.pVA=&msg->FIELDNAME; fi.szName=const_cast<char*>(QUOTEDFIELDNAME); fi.setPE((TEnumDescription*)new ENUMDESCR(&msg->FIELDNAME)); fi.setPD(nullptr); 
#define D_TIME(FIELDNAME, QUOTEDFIELDNAME)		return 0; } if (ix == ++i) {fi.eFT=EFT_TIME; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); typedef double T; fi.setPE(nullptr); fi.setPD(nullptr); 

#define D_BITFIELD(OBJNAME, QUOTEDFIELDNAME)	return 0; } if (ix == ++i) {fi.eFT=EFT_BITFIELD; fi.pVA=&OBJNAME; fi.szName = QUOTEDFIELDNAME; fi.setPE(nullptr); fi.setPD(nullptr); 

#define D_BOOL_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)	return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_BOOL; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef bool T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_CHAR_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)	return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_CHAR; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef char T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_UCHAR_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)  return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_UCHAR; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef uchar T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_SHORT_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)  return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_SHORT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef short T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_USHORT_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE) return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_USHORT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef ushort T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_INT_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)	return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_INT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef int T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_UINT_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)	return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_UINT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef uint T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_LONG_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)	return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_LONG; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef long T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_ULONG_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)	return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_ULONG; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef ulong T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_FLOAT_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE)  return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_FLOAT; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef float T; fi.setPE(nullptr); fi.setPD(nullptr); 
#define D_DOUBLE_ARRAY(FIELDNAME, QUOTEDFIELDNAME, ARRSIZE) return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_DOUBLE; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; typedef double T; fi.setPE(nullptr); fi.setPD(nullptr); 

#define D_CHARSTRING(FIELDNAME, QUOTEDFIELDNAME, CHARSIZE)  return 0; } if (ix == ++i) {fi.eFT=EFT_CHAR_STRING; fi.pVA=&msg->FIELDNAME; fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=CHARSIZE; typedef char* T; fi.setPE(nullptr); fi.setPD(nullptr); 

#define D_STRUCT_MEM(FIELDNAME, QUOTEDFIELDNAME, STRUCTTYPE, ARRSIZE)		return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_STRUCT; \
                            fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=msg->ARRSIZE; fi.setPE(nullptr); fi.setPD((TDescription*)new STRUCTTYPE(&msg->FIELDNAME[0])); 
#define D_STRUCT_CONST(FIELDNAME, QUOTEDFIELDNAME, STRUCTTYPE, ARRSIZE)		return 0; } if (ix == ++i) {fi.isArray = true; fi.eFT=EFT_STRUCT; \
                            fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=ARRSIZE; fi.setPE(nullptr); fi.setPD((TDescription*)new STRUCTTYPE(&msg->FIELDNAME[0])); 
#define D_STRUCT(FIELDNAME, QUOTEDFIELDNAME, STRUCTTYPE)					return 0; } if (ix == ++i) {fi.eFT=EFT_STRUCT; \
                            fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=1; fi.setPE(nullptr); fi.setPD((TDescription*)new STRUCTTYPE(&msg->FIELDNAME)); 
#define D_BASETYPE(QUOTEDFIELDNAME, STRUCTTYPE)								return 0; } if (ix == ++i) {fi.eFT=EFT_STRUCT; \
                            fi.szName = const_cast<char*>(QUOTEDFIELDNAME); fi.iSize=1; fi.setPE(nullptr); fi.setPD((TDescription*)new STRUCTTYPE(msg));

// описание семантики поля
#define D_LOGIC(TYPE)							fi.logicType |= TYPE;

// задание списка допустимых значений (через запятую)
// - использовать для типов float и double не рекомендуется
#define D_VALIDS(...)							fi.vals = new vector<T>; vector<T> v = {__VA_ARGS__}; v.swap(*(vector<T>*)fi.vals);
#define D_INVALIDS(...)							fi.vals = new vector<T>; vector<T> v = {__VA_ARGS__}; v.swap(*(vector<T>*)fi.vals); fi.invalidVals = true;

//! Задание диапазона допустимых значений
//! - использовать для типов float и double только с запасом по точности
#define D_MIN(MINVALUE)							fi.minval = new T(MINVALUE);
#define D_MAX(MAXVALUE)							fi.maxval = new T(MAXVALUE);
#define D_MINMAX(MINVALUE, MAXVALUE)			fi.minval = new T(MINVALUE); fi.maxval = new T(MAXVALUE);

#define DESCR_END return 0; } if (ix == 0) return static_cast<int>(i); return -1; }; int getObjSize() {return sizeof(*this); }; };

/******************************************************************************
@class TDescription descr.h

Пример использования макросов для определения наследников TDescription:
@code{.cpp}
	DESCR_START("cond_point", TDB_cond_point, DDB_cond_point)
		D_LONG(cp_id, "id")
		D_ENUM(old_enum_field, "old_enum")
		D_ENUMTYPE(enum_field, "enum_field", DEnumTypeDescr)
		D_BITFIELD(accessor_obj, "ready")
		D_TIME(time, "hours")
		D_BOOL_ARRAY(boolArr, "boolArr", 8)
		D_CHARSTRING(cp_text, "text", 50)
		D_STRUCT_MEM (runways, "runways", DRunway, uiRunwayCount)
		D_STRUCT_CONST (runways_2, "runways_2", DRunway, 2)
		D_STRUCT (_char, "aircraft", DAircraft)
	DESCR_END
@endcode

*******************************************************************************
Пример определенного макросами TDescription для конкретного типа данных:
@code{.cpp}
	//DESCR_START("cond_point", TDB_cond_point, DDB_cond_point)
	struct DDB_cond_point: public TDescription
	{
		TDB_cond_point *base, *msg;
		
		void *getBase() 
		{return reinterpret_cast<void*>(base);}; 
		
		DDB_cond_point(TDB_cond_point *pMsg) 
		{ 
			base = pMsg; 
			this->setName("cond_point"); 
		}
		
		int getField(size_t ix, DFieldInfo &fi, size_t iArrayShift) 
		{ 
			//if (ix < 0) return -1;
			
			size_t i = 0; 
			fi.iSize=1; 
			fi.setPD(nullptr); 
			fi.setPE(nullptr); 
			msg = base + iArrayShift; 
			
			if (false) 
			{
				
			// D_LONG(cp_id, "id")
				return 0; 
			} 
			if (ix == ++i) 
			{
				fi.eFT=EFT_LONG; 
				fi.pVA=&msg->cp_id; 
				fi.szName = const_cast<char*>("id"); 
				typedef int T;
			// D_ENUM(old_enum_field, "old_enum")
				return 0;
			} 
			if (ix == ++i) 
			{
				fi.eFT=EFT_ENUM; 
				fi.pVA=&msg->old_enum_field; 
				fi.szName = const_cast<char*>("old_enum"); 
				typedef int T;
			// D_ENUMTYPE(enum_field, "enum_field", DEnumTypeDescr)
				return 0; 
			} 
			if (ix == ++i) 
			{
				fi.eFT=EFT_ENUM; 
				fi.pVA=&msg->enum_field; 
				fi.szName=const_cast<char*>("enum_field"); 
				fi.setPE( (TEnumDescription*)new DEnumTypeDescr(&msg->enum_field) ); 
				fi.pVA=&msg->enum_field; 
			// D_TIME(time, "hours")
				return 0;
			} 
			if (ix == ++i) 
			{
				fi.eFT=EFT_TIME;
				fi.pVA=&msg->time; 
				fi.szName = const_cast<char*>("hours"); 
				typedef double T;
			// D_BITFIELD(accessor_obj, "ready")
				return 0;
			} 
			if (ix == ++i) 
			{
				fi.eFT=EFT_BITFIELD; 
				fi.pVA=&accessor_obj; 
				fi.szName = "ready";
			// D_BOOL_ARRAY(boolArr, "boolArr", 8)  
				return 0;
			} 
			if (ix == ++i) 
			{
				fi.isArray = true;
				fi.eFT=EFT_BOOL;
				fi.pVA=&msg->boolArr;
				fi.szName = const_cast<char*>("boolArr"); 
				fi.iSize=8; 
				typedef bool T;
			// D_CHARSTRING(cp_text, "text", 50)
				return 0; 
			} 
			if (ix == ++i) 
			{
				fi.eFT=EFT_CHAR_STRING; 
				fi.pVA=&msg->cp_text; 
				fi.szName = const_cast<char*>("text");
				fi.iSize=50; 
				typedef char* T;
			// D_STRUCT_MEM (runways, "runways", DRunway, uiRunwayCount)
				return 0; 
			} 
			if (ix == ++i) 
			{
				fi.isArray = true; 
				fi.eFT=EFT_STRUCT; 
				fi.setPD((TDescription*)new DRunway(&base->runways[iArrayShift]));
				fi.szName = const_cast<char*>("runways"); 
				fi.iSize=base->uiRunwayCount;
			// D_STRUCT_CONST (runways_2, "runways_2", DRunway, 2)
				return 0; 
			} 
			if (ix == ++i) 
			{
				fi.isArray = true; 
				fi.eFT=EFT_STRUCT; 
				fi.setPD((TDescription*)new DRunway(&base->runways_2[iArrayShift]));
				fi.szName = const_cast<char*>("runways_2"); 
				fi.iSize=2;
			// D_STRUCT (_char, "aircraft", DAircraft)
				return 0; 
			} 
			if (ix == ++i) 
			{
				fi.eFT=EFT_STRUCT; 
				fi.setPD((TDescription*)new DAircraft(&msg->_char)); 
				fi.szName = const_cast<char*>("aircraft"); 
				fi.iSize=1;
			// D_BASETYPE("baseClass", DBaseType)
				return 0; 
			} 
			if (ix == ++i) 
			{
				fi.eFT=EFT_STRUCT; 
				fi.setPD((TDescription*)new DBaseType(base));
				fi.szName = const_cast<char*>("baseClass");
				fi.iSize=1;
				
			// ИНТЕРФЕЙС НИЖЕ НЕ ПРОТЕСТИРОВАН !!!
			// задание списка допустимых значений (через запятую)
			// - использовать для типов float и double не рекомендуется
			//D_VALIDS(...)
				fi.vals = new vector<T>; 
				vector<T> v = {__VA_ARGS__}; 
				v.swap(*(vector<T>*)fi.vals);
			//D_INVALIDS(...)
				fi.vals = new vector<T>;
				vector<T> v = {__VA_ARGS__};
				v.swap(*(vector<T>*)fi.vals);
				fi.invalidVals = true;
				
			// задание диапазона допустимых значений
			// - использовать для типов float и double только с запасом по точности
			//D_MIN(MINVALUE)
				fi.minval = new T(MINVALUE);
			//D_MAX(MAXVALUE)
				fi.maxval = new T(MAXVALUE);
			//D_MINMAX(MINVALUE, MAXVALUE)
				fi.minval = new T(MINVALUE); 
				fi.maxval = new T(MAXVALUE);
	
			// DESCR_END
				return 0; 
			} 
			if (ix == 0) 
				return static_cast<int>(i); 
			return -1; 
		}; 
		
		int getObjSize() {return sizeof(*this); }; 
	};
@endcode
******************************************************************************/



#define ENUM_START(STRINGNAME, ENUMTYPE, RAWTYPE, DESCRTYPE, /* COMMENT (optional) */ ... ) struct DESCRTYPE : public TEnumDescription { ENUMTYPE *base; DESCRTYPE(ENUMTYPE *pVal) { base = pVal; } void *getBase() { return (void*)base; }; size_t getSize() { return sizeof(RAWTYPE); }; RAWTYPE	getRawValue(size_t iArrayShift = 0) { return (base) ? *(RAWTYPE*)shiftAddr(getType(), base, iArrayShift) : (RAWTYPE)0; } ulong	getValue(size_t iArrayShift = 0) { return (ulong)getRawValue(iArrayShift); } char *getName() { return STRINGNAME; }; const char *getComment() {return "" __VA_ARGS__;}; EDescrFieldType getType() { return typeinfo2enum(typeid(RAWTYPE)); }; \
	int getEntry(int ix, ulong val, ulong *pNumVal = nullptr, char **szShortStr = nullptr, char **szFullStr = nullptr) { int i = 0;

#define D_ENUMVALUE(ENUMVALUE, SHORTNAME, STRINGNAME)  if (ix == ++i || (ix == -1 && val == (ulong) ENUMVALUE )) { if (pNumVal) *pNumVal = (ulong) ENUMVALUE;  if (szShortStr) *szShortStr = SHORTNAME; if (szFullStr) *szFullStr = STRINGNAME; return 0; }
#define D_ENUMVAL(ENUMVALUE, SHORTNAME)  if (ix == ++i || (ix == -1 && val == (ulong) ENUMVALUE )) { if (pNumVal) *pNumVal = (ulong) ENUMVALUE;  if (szShortStr) *szShortStr = SHORTNAME; if (szFullStr) *szFullStr = SHORTNAME; return 0; }

#define ENUM_END if (ix == 0) return i; return -1; } };


/******************************************************************************
@class TEnumDescription descr.h

Пример использования макросов для определения наследников TEnumDescription:
@code{.cpp}
	enum class EClass4Descr : uchar {
		E_VAL_1,
		E_VAL_2
	}

	ENUM_START("NameEnumDescr", EClass4Descr, uchar, DEnumDescr)
		D_ENUMVALUE(EClass4Descr::E_VAL_1, "V1", "VAL 1")
		D_ENUMVAL(EClass4Descr::E_VAL_2, "V2")
	ENUM_END
@endcode
*******************************************************************************
Пример определенного макросами класса DEnumDescr для конкретного перечислимого типа:
@code{.cpp}
											// COMMENT (optional)	↓↓↓
	 //ENUM_START("NameEnumDescr", EClass4Descr, uchar, DEnumDescr, ... ) 
	 struct DEnumDescr : public TEnumDescription 
	 {		
		EClass4Descr *base; 
		
		DEnumDescr(EClass4Descr *pVal) 
		{ base = pVal; } 
		
		void *getBase() 
		{ return (void*)base; }; 
		
		size_t	getSize()		{ return sizeof(uchar); };
		uchar	getRawValue(size_t iArrayShift = 0) 
		{ 
			return (base) ? *(uchar*)shiftAddr(getType(), base, iArrayShift) : (uchar)0;
		} 
		ulong	getValue(size_t iArrayShift = 0) 
		{ 
			return (ulong)getRawValue(iArrayShift);
		} 
		char *getName() { return "NameEnumDescr"; }; 
		const char *getComment() { return "" __VA_ARGS__; }; 
		EDescrFieldType getType() { return typeinfo2enum(typeid(uchar)); }; 
		
		int getEntry(int ix, ulong val, ulong *pNumVal = nullptr, char **szShortStr = nullptr, char **szFullStr = nullptr) 
		{
			int i = 0;

			//D_ENUMVALUE(EClass4Descr::E_VAL_1, "V1", "VAL 1")  
			if (ix == ++i || (ix == -1 && val == (ulong) EClass4Descr::E_VAL_1 )) 
			{ 
				if (pNumVal) *pNumVal = (ulong) EClass4Descr::E_VAL_1;  
				if (szShortStr) *szShortStr = "V1"; 
				if (szFullStr) *szFullStr = "VAL 1"; 
				return 0; 
			}
			//D_ENUMVAL(ENUMVALUEEClass4Descr::E_VAL_2, "V2")
			if (ix == ++i || (ix == -1 && val == (ulong) EClass4Descr::E_VAL_2 )) 
			{
				if (pNumVal) *pNumVal = (ulong) EClass4Descr::E_VAL_2;  
				if (szShortStr) *szShortStr = "V2"; 
				if (szFullStr) *szFullStr = "V2"; 
				return 0; 
			}

			//ENUM_END 
			if (ix == 0) return i; return -1; 
		} 
	};
@endcode
******************************************************************************/


#endif // descrH

//---------------------------------------------------------------------------
