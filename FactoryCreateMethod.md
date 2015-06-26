# Signature #

`Object GearsFactory.create(string className, string classVersion)`

# Summary #

Creates a Gears object of the given class.

# Parameters #

| **Name** | **Type** | **Description** |
|:---------|:---------|:----------------|
| className | string   | Name of object to create |
| classVersion | string   | Deprecated. There is no longer any need to pass this parameter. The only allowed value is `'1.0'`. (To see if the machine has the minimum version of Gears you require, use `factory.version` instead.) |

# Return Value #

The newly created object.

# Details #

An exception is thrown if the given className is not recognized. The supported class names are:

| **className** | **Google Gears class created** |
|:--------------|:-------------------------------|
| beta.database | Database                       |
| beta.desktop  | Desktop                        |
| beta.httprequest | HttpRequest                    |
| beta.localserver | LocalServer                    |
| beta.timer    | Timer                          |
| beta.workerpool | WorkerPool                     |