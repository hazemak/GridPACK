// -------------------------------------------------------------
/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
// -------------------------------------------------------------
/**
 * @file   lpfile_optimizer_implementation.cpp
 * @author William A. Perkins
 * @date   2015-09-21 14:23:36 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#include <iostream>
#include <boost/assert.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include "gridpack/utilities/exception.hpp"
#include "lpfile_optimizer_implementation.hpp"

namespace gridpack {
namespace optimization {

// -------------------------------------------------------------
//  class LPFileVarVisitor
// -------------------------------------------------------------
class LPFileVarVisitor 
  : public VariableVisitor
{
public:

  /// Default constructor.
  LPFileVarVisitor(std::ostream& o)
    : VariableVisitor(),
      p_stream(o)
  {
  }

  /// Destructor
  ~LPFileVarVisitor(void)
  {
  }

protected:

  std::ostream& p_stream;

};

// -------------------------------------------------------------
// class LPFileVarBoundsLister
// -------------------------------------------------------------
class LPFileVarBoundsLister 
  : public LPFileVarVisitor
{
public:

  /// Default constructor.
  LPFileVarBoundsLister(std::ostream& o)
    : LPFileVarVisitor(o)
  {}

  /// Destructor
  ~LPFileVarBoundsLister(void)
  {}

  void visit(Variable& var)
  {
    BOOST_ASSERT(false);
  }

  void visit(RealVariable& var)
  {
    std::string s;
    s += "";
    if (var.bounded()) {
      if (var.lowerBound() > var.veryLowValue) {
        s += boost::str(boost::format("%8.4g") % var.lowerBound());
        s += " <= ";
      } else {
        s += "        ";
        s += "    ";
      }
      s += boost::str(boost::format("%-4.4s") % var.name());
      if (var.upperBound() < var.veryHighValue) {
        s += " <= ";
        s += boost::str(boost::format("%8.4g") % var.upperBound());
      } else {
        s += "        ";
        s += "    ";
      }
    } else {
      s += "        ";
      s += "    ";
      s += boost::str(boost::format("%-4.4s") % var.name());
      s += " free";
    }
    this->p_stream << s << std::endl;
  }
    
  void visit(IntegerVariable& var)
  {
    std::string s;
    s = "";
    if (var.lowerBound() > var.veryLowValue) {
      s += boost::str(boost::format("%8d") % var.lowerBound());
      s += " <= ";
    } else {
      s += "        ";
      s += "    ";
    }
    s += boost::str(boost::format("%-4.4s") % var.name());
    if (var.upperBound() < var.veryHighValue) {
      s += " <= ";
      s += boost::str(boost::format("%8d") % var.upperBound());
    } else {
      s += "    ";
      s += "        ";
    }
    this->p_stream << s << std::endl;
  }
};

// -------------------------------------------------------------
//  class LPFileGenVarLister
// -------------------------------------------------------------
class LPFileGenVarLister 
  : public LPFileVarVisitor
{
public:

  /// Default constructor.
  LPFileGenVarLister(std::ostream& o)
    : LPFileVarVisitor(o)
  {
  }

  /// Destructor
  ~LPFileGenVarLister(void)
  {
  }

  void visit(RealVariable& var)
  {
    this->p_stream << " " << var.name();
  };
};

// -------------------------------------------------------------
//  class LPFileIntVarLister
// -------------------------------------------------------------
class LPFileIntVarLister 
  : public LPFileVarVisitor
{
public:

  /// Default constructor.
  LPFileIntVarLister(std::ostream& o)
    : LPFileVarVisitor(o)
  {
  }

  /// Destructor
  ~LPFileIntVarLister(void)
  {
  }

  void visit(IntegerVariable& var)
  {
    this->p_stream << " " << var.name();
  };
};

// -------------------------------------------------------------
//  class LPFileBinVarLister
// -------------------------------------------------------------
class LPFileBinVarLister 
  : public LPFileVarVisitor
{
public:

  /// Default constructor.
  LPFileBinVarLister(std::ostream& o)
    : LPFileVarVisitor(o)
  {
  }

  /// Destructor
  ~LPFileBinVarLister(void)
  {
  }

  void visit(BinaryVariable& var)
  {
    this->p_stream << " " << var.name();
  };
};

// -------------------------------------------------------------
//  class LPFileConstraintRenderer
// -------------------------------------------------------------
class LPFileConstraintRenderer 
  : public ExpressionVisitor
{
public:

  /// Default constructor.
  LPFileConstraintRenderer(std::ostream& out)
    : ExpressionVisitor(), p_out(out)
  {}

  /// Destructor
  ~LPFileConstraintRenderer(void)
  {}

  void visit(IntegerConstant& e)
  { 
    p_out << boost::str(boost::format("%d") % e.value());
  }

  void visit(RealConstant& e)
  { 
    p_out << boost::str(boost::format("%g") % e.value());
  }

  void visit(VariableExpression& e)
  { 
    p_out << e.name();
  }

  void visit(UnaryExpression& e)
  {
    // should not be called
    BOOST_ASSERT(false);
  }

  void visit(UnaryMinus& e)
  {
    p_out << "-";
    if (e.rhs()->precedence() > e.precedence()) {
      p_group(e.rhs());
    } else {
      e.rhs()->accept(*this);
    }
  }

  void visit(UnaryPlus& e)
  {
    p_out << "+";
    if (e.rhs()->precedence() > e.precedence()) {
      p_group(e.rhs());
    } else {
      e.rhs()->accept(*this);
    }
  }

  void visit(BinaryExpression& e)
  {
    ExpressionPtr lhs(e.lhs());
    ExpressionPtr rhs(e.rhs());

    if (lhs->precedence() > e.precedence()) {
      p_group(lhs);
    } else {
      lhs->accept(*this);
    }
    p_out << " " << e.op() << " ";
    if (rhs->precedence() > e.precedence()) {
      p_group(rhs);
    } else {
      rhs->accept(*this);
    }
  }


  void visit(Multiplication& e)
  {
    ExpressionPtr lhs(e.lhs());
    ExpressionChecker lcheck;
    lhs->accept(lcheck);

    ExpressionPtr rhs(e.rhs());
    ExpressionChecker rcheck;
    rhs->accept(rcheck);

    // check to see that the LHS is a constant, otherwise it's a
    // nonlinear expression that is not handled by the LP language
    if (!lcheck.isConstant) { 
      BOOST_ASSERT_MSG(false, "LPFileConstraintRenderer: Invalid LHS to Multiplication");
    }

    // consider switching the LHS and RHS if the RHS is constant; for
    // now, the user should write correctly.

    lhs->accept(*this);         // it's a constant, right?
    p_out << " ";
    if (rhs->precedence() > e.precedence()) {
      p_group(rhs);
    }
  }

  void visit(Division& e)
  {
    ExpressionPtr lhs(e.lhs());
    ExpressionChecker lcheck;
    lhs->accept(lcheck);

    ExpressionPtr rhs(e.rhs());
    ExpressionChecker rcheck;
    rhs->accept(rcheck);

    // check to see that the RHS is a constant, otherwise it's a
    // nonlinear expression that is not handled by the LP language
    if (!lcheck.isConstant) { 
      BOOST_ASSERT_MSG(false, "LPFileConstraintRenderer: Invalid RHS to Division");
    }

    // consider switching the LHS and RHS if the RHS is constant; for
    // now, the user should write correctly.

    if (lhs->precedence() > e.precedence()) {
      p_group(lhs);
    } else {
      lhs->accept(*this);
    }
    p_out << "/";
    rhs->accept(*this);         // it's a constant, right?
  }  

  void visit(Addition& e)
  {
    this->visit(static_cast<BinaryExpression&>(e));
  }

  void visit(Subtraction& e)
  {
    ExpressionPtr lhs(e.lhs());
    ExpressionPtr rhs(e.rhs());

    if (lhs->precedence() > e.precedence()) {
      p_group(lhs);
    } else {
      lhs->accept(*this);
    }
    p_out << " - ";
    if (rhs->precedence() >= e.precedence()) {
      p_group(rhs);
    } else {
      rhs->accept(*this);
    }
  }
  void visit(Exponentiation& e)
  {
    ExpressionPtr lhs(e.lhs());

    ExpressionPtr rhs(e.rhs());
    ExpressionChecker rcheck;
    rhs->accept(rcheck);

    // Only integer constants are allowed as exponents -- check that
    if (!rcheck.isInteger) {
      BOOST_ASSERT_MSG(false, "LPFileConstraintRenderer: Only integer exponents allowed");
    }

    if (lhs->precedence() > e.precedence()) {
      p_group(lhs);
    } else {
      lhs->accept(*this);
    }
    p_out << "^";
    rhs->accept(*this);         // it's a constant, right?
  }

  void visit(Constraint& e)
  {
    ExpressionPtr lhs(e.lhs());
    ExpressionPtr rhs(e.rhs());

    p_out << "  " << e.name() << ": ";
    if (lhs->precedence() > e.precedence()) {
      p_group(lhs);
    } else {
      lhs->accept(*this);
    }
    p_out << " " << e.op() << " ";
    if (rhs->precedence() > e.precedence()) {
      p_group(rhs);
    } else {
      rhs->accept(*this);
    }
    p_out << std::endl;
  }

  void visit(LessThan& e)
  {
    this->visit(static_cast<Constraint&>(e));
  }

  void visit(LessThanOrEqual& e)
  {
    this->visit(static_cast<Constraint&>(e));
  }

  void visit(GreaterThan& e)
  {
    this->visit(static_cast<Constraint&>(e));
  }

  void visit(GreaterThanOrEqual& e)
  {
    this->visit(static_cast<Constraint&>(e));
  }

  void visit(Equal& e)
  {
    this->visit(static_cast<Constraint&>(e));
  }
  

protected:

  /// The stream to send renderings
  std::ostream& p_out;

  /// How to group an expression with higher precedence
  void p_group(ExpressionPtr e)
  {
    p_out << "[";
    e->accept(*this);
    p_out << "]";
  }
};

// -------------------------------------------------------------
//  class LPFileOptimizerImplementation
// -------------------------------------------------------------

// -------------------------------------------------------------
// LPFileOptimizerImplementation::p_temporaryFile
// -------------------------------------------------------------
std::string
LPFileOptimizerImplementation::p_temporaryFileName(void)
{
  using namespace boost::filesystem;
  path model("gridpack%%%%.lp");
  path tmp(temp_directory_path());
  tmp /= unique_path(model);

  boost::system::error_code ec;
  file_status istat = status(tmp);
  std::string result(tmp.c_str());
  return result;
  
}

// -------------------------------------------------------------
// LPFileOptimizerImplementation::p_write
// -------------------------------------------------------------
void
LPFileOptimizerImplementation::p_write(const p_optimizeMethod& method, std::ostream& out)
{
  out << "\\* Problem name: Test\\*" << std::endl << std::endl;

  switch (method) {
  case Maximize:
    out << "Maximize" << std::endl;
    break;
  case Minimize:
    out << "Minimize" << std::endl;
    break;
  default:
    BOOST_ASSERT(false);
  }
  out << "    "
            << p_objective->render() 
            << std::endl << std::endl;

  out << "Subject To" << std::endl;
  {
    LPFileConstraintRenderer r(out);
    for_each(p_constraints.begin(), p_constraints.end(),
             boost::bind(&Constraint::accept, _1, boost::ref(r)));
  }
  out << std::endl;   


  out << "Bounds" << std::endl;
  {
    LPFileVarBoundsLister v(out);
    for_each(p_variables.begin(), p_variables.end(),
             boost::bind(&Variable::accept, _1, boost::ref(v)));

  }
  out << std::endl;

  out << "General" << std::endl;
  out << "    ";
  {
    LPFileGenVarLister v(out);
    for_each(p_variables.begin(), p_variables.end(),
             boost::bind(&Variable::accept, _1, boost::ref(v)));

  }
  out << std::endl << std::endl;

  out << "Integer" << std::endl;
  out << "    ";
  {
    LPFileIntVarLister v(out);
    for_each(p_variables.begin(), p_variables.end(),
             boost::bind(&Variable::accept, _1, boost::ref(v)));

  }
  out << std::endl << std::endl;

  out << "Binary" << std::endl;
  out << "    ";
  {
    LPFileBinVarLister v(out);
    for_each(p_variables.begin(), p_variables.end(),
             boost::bind(&Variable::accept, _1, boost::ref(v)));

  }
  out << std::endl << std::endl;


  out << "End" << std::endl;
}


// -------------------------------------------------------------
// LPFileOptimizerImplementation::p_solve
// -------------------------------------------------------------
void
LPFileOptimizerImplementation::p_solve(const p_optimizeMethod& method)
{
  p_write(method, std::cout);
}
    
  


} // namespace optimization
} // namespace gridpack

