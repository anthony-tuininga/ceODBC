/*-----------------------------------------------------------------------------
 * SetupTest.sql
 *   Creates the objects used for testing ceODBC with a PostgreSQL database.
 /---------------------------------------------------------------------------*/

create table TestNumbers (
    IntCol              integer not null,
    BigIntCol           bigint,
    DecimalCol          decimal(6, 2)
);

create table TestStrings (
    IntCol              integer not null,
    StringCol           varchar(20) not null,
    NullableCol         varchar(50)
);

create table TestExecuteMany (
    IntCol              integer not null,
    StringCol           varchar(50)
);

alter table TestExecuteMany add constraint TestExecuteMany_pk
primary key (IntCol);

create function sp_Test (
    a_InValue varchar(50),
    inout a_InOutValue bigint,
    out a_OutValue decimal(6, 2)
) returns record as $$
begin
    a_InOutValue := a_InOutValue * 2;
    a_OutValue := length(a_InValue) * 1.25;
end;
$$ LANGUAGE plpgsql;

create function sp_TestNoArgs()
returns void as $$
begin
    null;
end;
$$ LANGUAGE plpgsql;

delete from TestNumbers;

insert into TestNumbers
values (1, 25, 125.25);

insert into TestNumbers
values (2, 1234567890123456, 245.37);

insert into TestNumbers
values (3, 9876543210, 25.99);

insert into TestNumbers
values (4, 98765432101234, 445.79);

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

