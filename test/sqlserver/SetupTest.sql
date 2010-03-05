/*-----------------------------------------------------------------------------
 * SetupTest.sql
 *   Creates the objects used for testing ceODBC with a MySQL database.
 /---------------------------------------------------------------------------*/

create table TestNumbers (
    IntCol              integer not null,
    BigIntCol           bigint,
    FloatCol            float
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

commit;

