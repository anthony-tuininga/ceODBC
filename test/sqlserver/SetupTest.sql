/*-----------------------------------------------------------------------------
 * SetupTest.sql
 *   Creates the objects used for testing ceODBC with a MySQL database.
 /---------------------------------------------------------------------------*/

create table TestDates (
    IntCol              integer not null,
    DateCol             datetime not null,
    NullableCol         datetime
);

create table TestNumbers (
    IntCol              integer not null,
    BigIntCol           bigint,
    FloatCol            float
);

create table TestStrings (
    IntCol              integer not null,
    StringCol           varchar(20) not null,
    NullableCol         varchar(50)
);

create table TestExecuteMany (
    IntCol              integer not null,
    StringCol           varchar(50),
    primary key (IntCol)
);

create procedure sp_Test (
    @a_InValue           varchar(50),
    @a_InOutValue        bigint output,
    @a_OutValue          float output
) as
begin
    set @a_InOutValue = @a_InOutValue * 2
    set @a_OutValue = len(@a_InValue) * 1.25
end

create procedure sp_TestNoArgs as
begin
    select 1
end

create function ufn_StringLength (
    @a_InValue          varchar(50)
) returns bigint as
begin
    return len(@a_InValue)
end

delete from TestDates;

insert into TestDates
values (1, '2010-03-01 00:00:00', '2010-03-02 08:00:00');

insert into TestDates
values (2, '2010-03-02 08:00:00', '2010-03-03 16:00:00');

insert into TestDates
values (3, '2010-03-03 16:00:00', '2010-03-04 07:30:00');

insert into TestDates
values (4, '2010-03-04 07:30:00', '2010-03-05 09:28:16');

delete from TestNumbers;

insert into TestNumbers
values (1, 25, 5.2);

insert into TestNumbers
values (2, 1234567890123456, 25.1);

insert into TestNumbers
values (3, 9876543210, 37.8);

insert into TestNumbers
values (4, 98765432101234, 77.27);

delete from TestStrings;

insert into TestStrings
values (1, 'String 1', null);

insert into TestStrings
values (2, 'String 2B', 'Nullable One');

insert into TestStrings
values (3, 'String 3XX', null);

insert into TestStrings
values (4, 'String 4YYY', 'Nullable Two');

commit;

