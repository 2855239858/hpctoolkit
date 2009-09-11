// -*-Mode: C++;-*-

// * BeginRiceCopyright *****************************************************
//
// $HeadURL$
// $Id$
//
// -----------------------------------
// Part of HPCToolkit (hpctoolkit.org)
// -----------------------------------
// 
// Copyright ((c)) 2002-2009, Rice University 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
// * Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the distribution.
// 
// * Neither the name of Rice University (RICE) nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
// 
// This software is provided by RICE and contributors "as is" and any
// express or implied warranties, including, but not limited to, the
// implied warranties of merchantability and fitness for a particular
// purpose are disclaimed. In no event shall RICE or contributors be
// liable for any direct, indirect, incidental, special, exemplary, or
// consequential damages (including, but not limited to, procurement of
// substitute goods or services; loss of use, data, or profits; or
// business interruption) however caused and on any theory of liability,
// whether in contract, strict liability, or tort (including negligence
// or otherwise) arising in any way out of the use of this software, even
// if advised of the possibility of such damage. 
// 
// ******************************************************* EndRiceCopyright *

//***************************************************************************
//
// File:
//   $HeadURL$
//
// Purpose:
//   [The purpose of this file]
//
// Description:
//   [The set of functions, macros, etc. defined in the file]
//
//***************************************************************************

#ifndef prof_juicy_Prof_CCT_Tree_hpp 
#define prof_juicy_Prof_CCT_Tree_hpp

//************************* System Include Files ****************************

#include <iostream>

#include <string>
#include <vector>

#include <typeinfo>

#include <cstring> // for memcpy

//*************************** User Include Files ****************************

#include <include/uint.h>

#include "Struct-Tree.hpp"

#include "MetricDesc.hpp"
#include "Metric-IData.hpp"

#include "LoadMap.hpp"

#include <lib/isa/ISATypes.hpp>

#include <lib/prof-lean/hpcrun-fmt.h>
#include <lib/prof-lean/lush/lush-support.h>

#include <lib/xml/xml.hpp>

#include <lib/support/diagnostics.h>
#include <lib/support/NonUniformDegreeTree.hpp>
#include <lib/support/SrcFile.hpp>
#include <lib/support/Unique.hpp>

//*************************** Forward Declarations ***************************

inline std::ostream&
operator<<(std::ostream& os, const hpcrun_metricVal_t x)
{
  os << "[" << x.i << " " << x.r << "]";
  return os;
}


//***************************************************************************
// Tree
//***************************************************************************

namespace Prof {

namespace CallPath {
  class Profile;
}


namespace CCT {

class ANode;
class ADynNode;

class Tree: public Unique {
public:

  enum {
    // Output flags
    OFlg_Compressed      = (1 << 1), // Write in compressed format
    OFlg_LeafMetricsOnly = (1 << 2), // Write metrics only at leaves
    OFlg_Debug           = (1 << 3), // Debug: show xtra source line info
    OFlg_DebugAll        = (1 << 4), // Debug: (may be invalid format)
  };

  // N.B.: An easy implementation for now (but not thread-safe!)
  static uint raToCallsiteOfst; 

public:
  // -------------------------------------------------------
  // Constructor/Destructor
  // -------------------------------------------------------
  
  Tree(const CallPath::Profile* metadata);

  virtual ~Tree();

  // -------------------------------------------------------
  // Tree data
  // -------------------------------------------------------
  ANode*
  root() const
  { return m_root; }

  void
  root(ANode* x)
  { m_root = x; }

  bool
  empty() const
  { return (m_root == NULL); }

  const CallPath::Profile*
  metadata() const
  { return m_metadata; }
  
  // -------------------------------------------------------
  // Given a Tree, merge into 'this'
  // -------------------------------------------------------
  void
  merge(const Tree* y, uint x_newMetricBegIdx, uint y_newMetrics);

  // -------------------------------------------------------
  // Write contents
  // -------------------------------------------------------
  std::ostream& 
  writeXML(std::ostream& os = std::cerr, int oFlags = 0) const;

  std::ostream& 
  dump(std::ostream& os = std::cerr, int oFlags = 0) const;
  
  void
  ddump() const;


  // Given a set of flags 'flags', determines whether we need to
  // ensure that certain characters are escaped.  Returns xml::ESC_TRUE
  // or xml::ESC_FALSE. 
  static int
  doXMLEscape(int oFlags);
 
private:
  ANode* m_root;
  const CallPath::Profile* m_metadata; // does not own
};


} // namespace CCT

} // namespace Prof


//***************************************************************************
// ANode
//***************************************************************************

namespace Prof {

namespace CCT {


class ANode;     // Everyone's base class 

class Root; 

class ProcFrm;
class Proc; 
class Loop; 
class Stmt;
class Call;

// ---------------------------------------------------------
// ANode: The base node for a call stack profile tree.
// ---------------------------------------------------------
class ANode: public NonUniformDegreeTreeNode, 
	     public Unique {
public:
  enum ANodeTy {
    TyRoot = 0,
    TyProcFrm,
    TyProc,
    TyLoop,
    TyStmt,
    TyCall,
    TyANY,
    TyNUMBER
  };
  
  static const std::string&
  ANodeTyToName(ANodeTy tp);
  
  static ANodeTy
  IntToANodeType(long i);

private:
  static const std::string NodeNames[TyNUMBER];
  
public:
  ANode(ANodeTy type, ANode* parent, Struct::ACodeNode* strct = NULL)
    : NonUniformDegreeTreeNode(parent), m_type(type), m_strct(strct)
  { 
    static uint uniqueId = 2; // To support reading/writing, ids must be
    m_id = uniqueId += 2;     // consistent with HPCRUN_FMT_RetainIdFlag.
  }

  virtual ~ANode()
  { }
  
  // shallow copy (in the sense the children are not copied)
  ANode(const ANode& x)
    : m_type(x.m_type), m_strct(x.m_strct) // do not copy 'm_id'
  { 
    zeroLinks();
  }

  ANode& operator=(const ANode& x) 
  {
    if (this != &x) {
      m_type = x.m_type;
      m_strct = x.m_strct;
      // do not copy 'm_id'
    }
    return *this;
  }


  // --------------------------------------------------------
  // General data
  // --------------------------------------------------------
  ANodeTy
  type() const
  { return m_type; }

  // id: a unique id; 0 is reserved for a NULL value
  uint
  id() const
  { return m_id; }

  // 'name()' is overridden by some derived classes
  virtual const std::string&
  name() const
  { return ANodeTyToName(type()); }


  // structure: static structure id for this node; the same static
  // structure will have the same structure().
  Struct::ACodeNode*
  structure() const
  { return m_strct; }

  void
  structure(Struct::ACodeNode* strct)
  { m_strct = strct; }

  uint
  structureId() const
  { return (m_strct) ? m_strct->id() : 0; }

  SrcFile::ln
  begLine() const
  { return (m_strct) ? m_strct->begLine() : ln_NULL; }

  SrcFile::ln
  endLine() const
  { return (m_strct) ? m_strct->endLine() : ln_NULL;  }
  

  // --------------------------------------------------------
  // Tree navigation 
  // --------------------------------------------------------
  ANode*
  parent() const 
  { return static_cast<ANode*>(NonUniformDegreeTreeNode::Parent()); }

  ANode*
  firstChild() const
  { return static_cast<ANode*>(NonUniformDegreeTreeNode::FirstChild()); }

  ANode*
  lastChild() const
  { return static_cast<ANode*>(NonUniformDegreeTreeNode::LastChild()); }

  ANode*
  nextSibling() const
  {
    // siblings are linked in a circular list
    if ((parent()->lastChild() != this)) {
      return dynamic_cast<ANode*>(NonUniformDegreeTreeNode::NextSibling()); 
    }
    return NULL;  
  }

  ANode*
  prevSibling() const
  {
    // siblings are linked in a circular list
    if ((parent()->firstChild() != this)) {
      return dynamic_cast<ANode*>(NonUniformDegreeTreeNode::PrevSibling()); 
    }
    return NULL;
  }


  // --------------------------------------------------------
  // ancestor: find first ANode in path from this to root with given type
  // (Note: We assume that a node *can* be an ancestor of itself.)
  // --------------------------------------------------------
  ANode*
  ancestor(ANodeTy tp) const;
  
  Root*
  ancestorRoot() const;

  ProcFrm*
  ancestorProcFrm() const;

  Proc*
  ancestorProc() const;

  Loop*
  ancestorLoop() const;

  Stmt*
  ancestorStmt() const;

  Call*
  ancestorCall() const;


  // --------------------------------------------------------
  // merging
  // --------------------------------------------------------

  // mergeDeep: Let 'this' = x and let y be a node corresponding to x
  //   in the sense that we may think of y as being locally merged
  //   with x (according to ADynNode::isMergable()).  Given y,
  //   recursively merge y's children into x.
  // N.B.: assume we can destroy y.
  // N.B.: assume x already has space to store merged metrics
  void 
  mergeDeep(ANode* y, uint x_numMetrics, uint y_numMetrics);

  // merge: Let 'this' = x and let y be a node corresponding to x.
  //   Merge y into x.  
  // N.B.: assume we can destroy y.
  void 
  merge(ANode* y);
  

  // virtual void mergeLocal();


  CCT::ADynNode* 
  findDynChild(const ADynNode& y_dyn);

  void 
  merge_prepare(uint numMetrics);


  // --------------------------------------------------------
  // Output
  // --------------------------------------------------------
  virtual std::string 
  Types() const; // this instance's base and derived types

  std::string 
  toString(int oFlags = 0, const char* pre = "") const;

  virtual std::string 
  toString_me(int oFlags = 0) const; 

  std::ostream& 
  writeXML(std::ostream& os = std::cerr, int oFlags = 0,
	   const char* pre = "") const;

  std::ostream& 
  dump(std::ostream& os = std::cerr, int oFlags = 0, const char* pre = "") const;

  void
  ddump() const;

  void
  ddump_me() const;
  
  virtual std::string
  codeName() const;

protected:

  bool
  writeXML_pre(std::ostream& os = std::cerr, int oFlags = 0,
	       const char* prefix = "") const;
  void
  writeXML_post(std::ostream& os = std::cerr, int oFlags = 0, 
		const char* prefix = "") const;

  void
  merge_fixup(int newMetrics);
  
protected:
  ANodeTy m_type; // obsolete with typeid(), but hard to replace
  uint m_id;
  Struct::ACodeNode* m_strct;
};


// - if x < y; 0 if x == y; + otherwise
int ANodeLineComp(ANode* x, ANode* y);


//***************************************************************************
// ADynNode
//***************************************************************************

// ---------------------------------------------------------
// ANode: represents dynamic nodes
// ---------------------------------------------------------
class ADynNode : public ANode, public Metric::IData {
public:

  // -------------------------------------------------------
  // 
  // -------------------------------------------------------
  
  ADynNode(ANodeTy type, ANode* parent, Struct::ACodeNode* strct, uint cpId)
    : ANode(type, parent, strct),
      Metric::IData(),
      m_cpId(cpId),
      m_as_info(lush_assoc_info_NULL),
      m_lmId(LoadMap::LM_id_NULL), m_ip(0), m_opIdx(0), m_lip(NULL)
    { }

  ADynNode(ANodeTy type, ANode* parent, Struct::ACodeNode* strct,
	   uint32_t cpId, lush_assoc_info_t as_info, 
	   LoadMap::LM_id_t lmId, VMA ip, ushort opIdx, lush_lip_t* lip)
    : ANode(type, parent, strct),
      Metric::IData(),
      m_cpId(cpId),
      m_as_info(as_info), 
      m_lmId(lmId), m_ip(ip), m_opIdx(opIdx), m_lip(lip)
  { }

  ADynNode(ANodeTy type, ANode* parent, Struct::ACodeNode* strct,
	   uint32_t cpId, lush_assoc_info_t as_info, 
	   LoadMap::LM_id_t lmId, VMA ip, ushort opIdx, lush_lip_t* lip,
	   const Metric::IData& metrics)
    : ANode(type, parent, strct),
      Metric::IData(metrics),
      m_cpId(cpId),
      m_as_info(as_info),
      m_lmId(lmId), m_ip(ip), m_opIdx(opIdx), m_lip(lip)
  { }

  virtual ~ADynNode() 
  { delete m_lip; }
   
  // deep copy of internals (but without children)
  ADynNode(const ADynNode& x)
    : ANode(x),
      Metric::IData(x),
      m_cpId(x.m_cpId),
      m_as_info(x.m_as_info), 
      m_lmId(x.m_lmId),
      m_ip(x.m_ip), m_opIdx(x.m_opIdx), 
      m_lip(clone_lip(x.m_lip))
    { }

  // deep copy of internals (but without children)
  ADynNode&
  operator=(const ADynNode& x)
  {
    if (this != &x) {
      ANode::operator=(x);
      Metric::IData::operator=(x);
      m_cpId = x.m_cpId;
      m_as_info = x.m_as_info;
      m_lmId = x.m_lmId;
      m_ip = x.m_ip;
      m_opIdx = x.m_opIdx;
      delete m_lip;
      m_lip = clone_lip(x.m_lip);
    }
    return *this;
  }


  // -------------------------------------------------------
  // call path id
  // -------------------------------------------------------

  // call path id: a persistent call path id (persistent in the sense
  // that it must be consistent with hpctrace).  0 is reserved as a
  // NULL value.

  uint 
  cpId() const
  { return m_cpId; }
  

  // -------------------------------------------------------
  // logical unwinding association
  // -------------------------------------------------------

  lush_assoc_info_t 
  assocInfo() const 
  { return m_as_info; }

  void 
  assocInfo(lush_assoc_info_t x) 
  { m_as_info = x; }

  lush_assoc_t 
  assoc() const 
  { return lush_assoc_info__get_assoc(m_as_info); }


  // -------------------------------------------------------
  // load-module id, ip
  // -------------------------------------------------------

  LoadMap::LM_id_t
  lmId() const 
  {
    if (isValid_lip()) { return lush_lip_getLMId(m_lip); }
    return m_lmId; 
  }

  LoadMap::LM_id_t
  lmId_real() const
  { return m_lmId; }

  void
  lmId(LoadMap::LM_id_t x)
  { 
    if (isValid_lip()) { lush_lip_setLMId(m_lip, x); return; }
    m_lmId = x; 
  }

  void
  lmId_real(LoadMap::LM_id_t x)
  { m_lmId = x; }

  virtual VMA 
  ip() const 
  {
    if (isValid_lip()) { return lush_lip_getIP(m_lip); }
    return m_ip; 
  }

  VMA 
  ip_real() const 
  { return m_ip; }

  void 
  ip(VMA ip, ushort opIdx) 
  { 
    if (isValid_lip()) { lush_lip_setIP(m_lip, ip); m_opIdx = 0; return; }
    m_ip = ip; m_opIdx = opIdx;
  }

  ushort
  opIndex() const 
  { return m_opIdx; }


  // -------------------------------------------------------
  // logical ip
  // -------------------------------------------------------

  lush_lip_t*
  lip() const 
  { return m_lip; }
  
  void
  lip(lush_lip_t* lip) 
  { m_lip = lip; }

  static lush_lip_t*
  clone_lip(lush_lip_t* x) 
  {
    lush_lip_t* x_clone = NULL;
    if (x) {
      x_clone = new lush_lip_t;
      memcpy(x_clone, x, sizeof(lush_lip_t));
    }
    return x_clone;
  }

  bool
  isValid_lip() const
  { return (m_lip && (lush_lip_getLMId(m_lip) != 0)); }


  // -------------------------------------------------------
  // merging
  // -------------------------------------------------------

  static bool 
  isMergable(const ADynNode& x, const ADynNode& y)
  {
    // N.B.: order is based on early failure rather than logical order
    return (x.isLeaf() == y.isLeaf()
	    && x.lmId_real() == y.lmId_real()
	    && x.ip_real() == y.ip_real()
	    && lush_lip_eq(x.lip(), y.lip())
	    && lush_assoc_class_eq(x.assoc(), y.assoc())
	    && lush_assoc_info__path_len_eq(x.assocInfo(), y.assocInfo()));
  }

  void
  mergeLocal(const ADynNode& y, uint metricBegIdx = 0);
  

  // -------------------------------------------------------
  // Dump contents for inspection
  // -------------------------------------------------------

  std::string
  assocInfo_str() const;
  
  std::string
  lip_str() const;

  std::string
  nameDyn() const;

  void 
  writeDyn(std::ostream& os, int oFlags = 0, const char* prefix = "") const;

  // writeMetricsXML: write metrics (sparsely)
  std::ostream& 
  writeMetricsXML(std::ostream& os, int oFlags = 0, 
		  const char* prefix = "") const;


private:
  uint m_cpId; // dynamic id

  lush_assoc_info_t m_as_info;

  LoadMap::LM_id_t m_lmId; // LoadMap::LM id
  VMA m_ip;                // instruction pointer for this node
  ushort m_opIdx;          // index in the instruction [OBSOLETE]

  lush_lip_t* m_lip; // logical ip
};


//***************************************************************************
// 
//***************************************************************************

// --------------------------------------------------------------------------
// Root
// --------------------------------------------------------------------------

class Root: public ANode {
public: 
  // Constructor/Destructor
  Root(const std::string& nm);
  Root(const char* nm);
  virtual ~Root();

  const std::string&
  name() const { return m_name; }
  
  // Dump contents for inspection
  virtual std::string
  toString_me(int oFlags = 0) const;
  
protected: 
private: 
  std::string m_name; // the program name
}; 


// --------------------------------------------------------------------------
// ProcFrm
// --------------------------------------------------------------------------

class ProcFrm: public ANode {
public:
  // Constructor/Destructor
  ProcFrm(ANode* parent, Struct::ACodeNode* strct = NULL);

  virtual ~ProcFrm();

  // shallow copy (in the sense the children are not copied)
  ProcFrm(const ProcFrm& x)
    : ANode(x)
  { }

  // -------------------------------------------------------
  // Static structure
  // -------------------------------------------------------
  // NOTE: m_strct is either Struct::Proc or Struct::Alien

  const std::string& 
  lmName() const
  { 
    if (m_strct) { 
      return m_strct->ancestorLM()->name();
    }
    else {
      return BOGUS; 
    }
  }

  uint 
  lmId() const
  { return (m_strct) ? m_strct->ancestorLM()->id() : 0; }


  const std::string& 
  fileName() const
  {
    if (m_strct) {
      return (isAlien()) ? 
	dynamic_cast<Struct::Alien*>(m_strct)->fileName() :
	m_strct->ancestorFile()->name();
    }
    else {
      return BOGUS;
    }
  }

  uint 
  fileId() const
  {
    uint id = 0;
    if (m_strct) {
      id = (isAlien()) ? m_strct->id() : m_strct->ancestorFile()->id();
    }
    return id;
  }


  const std::string& 
  procName() const
  {
    // Struct::Proc or Struct::Alien
    if (m_strct) { 
      return m_strct->name();
    }
    else {
      return BOGUS; 
    }
  }


  std::string
  procNameDbg() const;


  uint 
  procId() const
  { return (m_strct) ? m_strct->id() : 0; }


  // Alien
  bool
  isAlien() const
  { return (m_strct && typeid(*m_strct) == typeid(Struct::Alien)); }

  // -------------------------------------------------------
  //
  // -------------------------------------------------------

  virtual std::string 
  toString_me(int oFlags = 0) const;

  virtual std::string 
  codeName() const;
 
private: 
  static std::string BOGUS;
};


// --------------------------------------------------------------------------
// Proc: FIXME: Not used!
// --------------------------------------------------------------------------

class Proc: public ANode {
public: 
  // Constructor/Destructor
  Proc(ANode* parent, Struct::ACodeNode* strct = NULL);
  virtual ~Proc();
  
  // Dump contents for inspection
  virtual std::string 
  toString_me(int oFlags = 0) const;
};


// --------------------------------------------------------------------------
// Loop
// --------------------------------------------------------------------------

class Loop: public ANode {
public: 
  // Constructor/Destructor
  Loop(ANode* parent, Struct::ACodeNode* strct = NULL);

  virtual ~Loop();

  // Dump contents for inspection
  virtual std::string 
  toString_me(int oFlags = 0) const; 
  
private:
};


// --------------------------------------------------------------------------
// Stmt
// --------------------------------------------------------------------------
  
class Stmt: public ADynNode {
 public:
  // Constructor/Destructor
  Stmt(ANode* parent, uint cpId)
    : ADynNode(TyStmt, parent, NULL, cpId)
  { }

  Stmt(ANode* parent,
       uint32_t cpId, lush_assoc_info_t as_info,
       LoadMap::LM_id_t lmId, VMA ip, ushort opIdx, lush_lip_t* lip,
       const Metric::IData& metrics)
    : ADynNode(TyStmt, parent, NULL, 
	       cpId, as_info, lmId, ip, opIdx, lip, metrics)
  { }

  virtual ~Stmt()
  { }

  Stmt&
  operator=(const Stmt& x) 
  {
    if (this != &x) {
      ADynNode::operator=(x);
    }
    return *this;
  }

  // Dump contents for inspection
  virtual std::string 
  toString_me(int oFlags = 0) const;
};


// --------------------------------------------------------------------------
// Call (callsite)
// --------------------------------------------------------------------------

class Call: public ADynNode {
public:
  // Constructor/Destructor
  Call(ANode* parent, uint cpId);

  Call(ANode* parent, 
       uint32_t cpId, lush_assoc_info_t as_info,
       LoadMap::LM_id_t lmId, VMA ip, ushort opIdx, lush_lip_t* lip,
       const Metric::IData& metrics);

  virtual ~Call() 
  { }
  
  // Node data
  virtual VMA 
  ip() const 
  {
    if (isValid_lip()) { return (lush_lip_getIP(lip()) - Tree::raToCallsiteOfst); }
    return (ADynNode::ip_real() - Tree::raToCallsiteOfst);
  }
  
  VMA 
  ra() const 
  { return ADynNode::ip_real(); }
    
  // Dump contents for inspection
  virtual std::string 
  toString_me(int oFlags = 0) const;

};


} // namespace CCT

} // namespace Prof


//***************************************************************************

#include "CCT-TreeIterator.hpp"

//***************************************************************************


#endif /* prof_juicy_Prof_CCT_Tree_hpp */
