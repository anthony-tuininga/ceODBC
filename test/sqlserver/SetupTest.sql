/*-----------------------------------------------------------------------------
 * SetupTest.sql
 *   Creates the objects used for testing ceODBC with a MySQL database.
 /---------------------------------------------------------------------------*/

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

