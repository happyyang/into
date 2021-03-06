/* This file is part of Into.
 * Copyright (C) Intopii 2013.
 * All rights reserved.
 *
 * Licensees holding a commercial Into license may use this file in
 * accordance with the commercial license agreement. Please see
 * LICENSE.commercial for commercial licensing terms.
 *
 * Alternatively, this file may be used under the terms of the GNU
 * Affero General Public License version 3 as published by the Free
 * Software Foundation. In addition, Intopii gives you special rights
 * to use Into as a part of open source software projects. Please
 * refer to LICENSE.AGPL3 for details.
 */

#ifndef _PIICLASSINDEXMAPPER_H
#define _PIICLASSINDEXMAPPER_H

#include <PiiDefaultOperation.h>

/**
 * An operation that maps indices into other indices. This operation
 * can make use of a special data structure, [classInfoMap], which
 * stores information about class indices. Its purpose is to map a
 * continuous range of class indices to a possibly non-continuos
 * range. The operation is useful if classes need to be deleted and
 * added while the remaining ones must retain their indices.
 *
 * Let us assume we initially have a continuous range of class indices
 * 0-3. After deleting class 1 we have only three classes left (0, 2,
 * 3). Since classifiers use a continuous range of indices (0, 1, 2)
 * we need to map 1 and 2 to 2 and 3 to retain the old indices. Here's
 * how:
 *
 * ~~~(c++)
 * PiiOperation* mapper = engine.createOperation("PiiClassIndexMapper");
 * mapper->setProperty("classIndexMap", QVariantList() << 0 << 2 << 3);
 * ~~~
 *
 * The operation makes it easy to connect additional information to
 * classes. Let us assume that we later changed the order of classes 0
 * and 2. Here's what we need to do:
 *
 * ~~~(c++)
 * // The normal stuff for converting class indices
 * mapper->setProperty("classIndexMap", QVariantList() << 2 << 0 << 3);
 *
 * // Include additional information
 * QVariantMap classInfoMap;
 * classInfoMap["classNames"] = QStringList() << "two" << "zero" << "three";
 * mapper->setProperty("classInfoMap", classInfoMap);
 * ~~~
 *
 * As a result, the `class name` output will emit "two" for any
 * incoming 0, "zero" for ones, "three" for twos, and "Unknown" for
 * anything else.
 *
 * It is also possible to map multiple indices into one but still
 * include the class information only once. Here's how:
 *
 * ~~~(c++)
 * // Mapping table:
 * // 0-2 -> 1
 * // 3   -> 5
 * // 4   -> 2
 * // 5   -> 0
 * mapper->setProperty("classIndexMap", QVariantList() << 1 << 1 << 1 << 5 << 2 << 0);
 *
 * QVariantMap classInfoMap;
 * // Now we only have four classes and we don't want to repeat the class names.
 * classInfoMap["classNames"] = QStringList() << "zero" << "one" << "two" << "five";
 * // These indices map the values in classIndexMap back to classNames
 * classInfoMap["classIndices"] = QVariantList() << 0 << 1 << 2 << 5;
 * mapper->setProperty("classInfoMap", classInfoMap);
 * ~~~
 *
 * The result:
 *
 * <table>
 * <tr><th>Input</th><th>class index</th><th>class name</th><th>list index</th></tr>
 * <tr><td>0</td><td>1</td><td>one</td><td>1</td></tr>
 * <tr><td>1</td><td>1</td><td>one</td><td>1</td></tr>
 * <tr><td>2</td><td>1</td><td>one</td><td>1</td></tr>
 * <tr><td>3</td><td>5</td><td>five</td><td>3</td></tr>
 * <tr><td>4</td><td>2</td><td>four</td><td>2</td></tr>
 * <tr><td>5</td><td>0</td><td>zero</td><td>0</td></tr>
 * </table>
 *
 * Inputs
 * ------
 *
 * @in class index - Input class index (any primitive type, converted
 * to an int)
 *
 * Outputs
 * -------
 *
 * @out class index - Converted class index as specified by
 * [classIndexMap] (int)
 *
 * @out class name - Class name, if [classInfoMap] provides such
 * information, "Unknown" otherwise. (QString)
 *
 * @out list index - the index of the class in the `classIndices`
 * list (see [classInfoMap]).
 *
 */
class PiiClassIndexMapper : public PiiDefaultOperation
{
  Q_OBJECT

  /**
   * Additional information for classes. The info map contains a
   * number of QVariantLists that provide additional information for
   * the classes. Each of the lists must be of equal length.
   *
   * - `classIndices` - indices of the classes. The indices may not
   * be zero-based and may skip values. A QVariantList whose elements
   * are convertible to ints. If this value is present, the indices in
   * [classIndexMap] are mapped back to list indices using the values
   * in this list. The output of `list index` is meaningful
   * only if this value is set.
   *
   * - `classNames` - the names of the classes. A QStringList.
   *
   * The map may also include other information, which is ignored by
   * the operation. All information will however be saved and restored
   * if the operation is serialized.
   */
  Q_PROPERTY(QVariantMap classInfoMap READ classInfoMap WRITE setClassInfoMap);

  /**
   * The input-output mapping. The length of this list should be equal
   * to the maximum possible input class index plus one. Each value in
   * this list specifies the output value for the corresponding input
   * class index. The following example configures the mapper so that
   * each incoming zero is converted to one and vice versa. If the
   * input index is below zero or exceeds the size of the class index
   * map, zero will be emitted.
   *
   * ~~~(c++)
   * // Map zero to one and one to zero
   * mapper->setProperty("classIndexMap", QVariantList() << 1 << 0);
   * ~~~
   */
  Q_PROPERTY(QVariantList classIndexMap READ classIndexMap WRITE setClassIndexMap);

  PII_OPERATION_SERIALIZATION_FUNCTION
public:
  PiiClassIndexMapper();

  void setClassInfoMap(const QVariantMap& classInfoMap);
  QVariantMap classInfoMap() const;

  void setClassIndexMap(const QVariantList& classIndexMap);
  QVariantList classIndexMap() const;

protected:
  void process();

private:
  /// @internal
  class Data : public PiiDefaultOperation::Data
  {
  public:
    QVariantMap classInfoMap;

    QList<int> lstClassIndices;
    QList<int> lstListIndices;
    QStringList lstClassNames;
  };
  PII_D_FUNC;
};


#endif //_PIICLASSINDEXMAPPER_H
