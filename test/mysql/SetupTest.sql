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
);

delete from TestNumbers;

insert into TestNumbers
values (1, 25, 5.2, 7.3, 125.25);

insert into TestNumbers
values (2, 1234567890123456, 25.1, 17.8, 245.37);

insert into TestNumbers
values (3, 9876543210, 37.8, 235.19, 25.99);

insert into TestNumbers
values (4, 98765432101234, 77.27, 922.78, 445.79);

commit;

