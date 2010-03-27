/*-----------------------------------------------------------------------------
 * SetupTest.sql
 *   Creates the objects used for testing ceODBC with a MySQL database.
 /---------------------------------------------------------------------------*/

create table TestNumbers (
    IntCol              integer not null,
    BigIntCol           bigint,
    FloatCol            float(5, 2),
    DoubleCol           double(7, 2),
    DecimalCol          decimal(6, 2)
) engine=InnoDB;

create table TestStrings (
    IntCol              integer not null,
    StringCol           varchar(20) not null,
    NullableCol         varchar(50)
) engine=InnoDB;

create table TestExecuteMany (
    IntCol              integer not null,
    StringCol           varchar(50)
) engine=InnoDB;

alter table TestExecuteMany add constraint TestExecuteMany_pk
primary key (IntCol);

DELIMITER //
create procedure sp_Test (
    a_InValue varchar(50),
    inout a_InOutValue bigint,
    out a_OutValue decimal(6, 2)
)
begin
    set a_InOutValue = a_InOutValue * 2;
    set a_OutValue = len(a_InValue) * 1.25;
end //

create procedure sp_TestNoArgs()
begin
    select 1;
end //

DELIMITER ;

delete from TestNumbers;

insert into TestNumbers
values (1, 25, 5.2, 7.3, 125.25);

insert into TestNumbers
values (2, 1234567890123456, 25.1, 17.8, 245.37);

insert into TestNumbers
values (3, 9876543210, 37.8, 235.19, 25.99);

insert into TestNumbers
values (4, 98765432101234, 77.27, 922.78, 445.79);

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

