.. _varobj:

****************
Variable Objects
****************

.. note::

   The DB API definition does not define this object.


.. attribute:: Variable.bufferSize

   This read-only attribute returns the size of the buffer allocated for each
   element.


.. method:: Variable.getvalue([pos=0])

   Return the value at the given position in the variable.


.. attribute:: Variable.input

   This read-write attribute specifies whether the variable is used as an input
   variable and should normally be left as True.


.. attribute:: Variable.numElements

   This read-only attribute returns the number of elements allocated.


.. attribute:: Variable.output

   This read-write attribute specifies whether the variable is used as an
   output variable. It should normally be left as False except when calling
   stored procedures with output variables.


.. attribute:: Variable.scale

   This read-only attribute returns the scale of the variable.


.. method:: Variable.setvalue(pos, value)

   Set the value at the given position in the variable.


.. attribute:: Variable.size

   This read-only attribute returns the size of the variable.

