﻿
#include "planningMap.h"



namespace pm {
	Map3D::Map3D()
	{
		std::cout << "map begin to init" << endl;
	}
	Map3D::~Map3D()
	{
	}
	void Map3D::setGridRange(double const& min_x, double const min_y, double const& max_x, double const& max_y)
	{
		this->mWsPoint1.x(min_x);
		this->mWsPoint1.y(min_y);
		this->mWsPoint3.x(max_x);
		this->mWsPoint3.y(max_y);
	}

	void Map3D::setGridSize(double const& aSize, double const& sSize)
	{
		//如果不能整除怎么办？
			this->AgridStep = aSize; 
			this->SgridStep = sSize;
	}

	bool Map3D::setOriginData(vector<double> const& _data, double const& _originGridSize, size_t const& _originColSize)
	{
		this->_m_originData = _data;
		this->_m_originGridSize = _originGridSize;
		
		this->_m_originColSize = _originColSize;
		this->_m_originRowSize = _data.size() / _originColSize;
		
		
		this->_m_vOriginData.clear();
		_m_vOriginData.assign(_m_originRowSize, std::vector<double>());
			
		for (size_t i = 0; i < _data.size(); i++)
		{
			size_t row = std::floor(i / _m_originColSize);
			//size_t col = i % _m_originColSize;
			//cout << "row = " << row << endl;
			_m_vOriginData[row].push_back(_data[i]);
		}



		// set range
		this->mWsPoint1.set<0>(0);
		this->mWsPoint1.set<1>(0);

		this->mWsPoint3.x(_originGridSize* _m_originColSize);
		this->mWsPoint3.y(_originGridSize* _m_originRowSize);
		return true;
	}

	void Map3D::addObRing(std::vector<double> const& vx, std::vector<double> const& vy)
	{
		auto ringUnit = bgeo::v2Ring(vx, vy);
		_m_vDRing.push_back(ringUnit);
	}

	bool Map3D::createMapGraph(){
		this->map2AGrid();
		this->grid2Graph();
		return true;
	}

	bool Map3D::empty(int const& t)
	{
		return false;
	}

	GridIndex Map3D::pnt2Index(bgeo::DPoint const& pnt, size_t const& _type)
	{
		GridIndex rindex;
		rindex.first = -1;
		rindex.second = -1;

		double _i_x = this->mWsPoint1.x();
		double _i_y = this->mWsPoint1.y();
		if (pnt.y() < _i_y)
			return rindex;
		if (pnt.x() < _i_x)
			return rindex;
		auto bais_x = pnt.x() - _i_x;
		auto bais_y = pnt.y() - _i_y;
		double step;

		if (_type == MapType::AggregationMap)
			step = this->AgridStep;
		else
			step = this->SgridStep;

		auto p_col = floor(bais_x / step);
		auto p_row = floor(bais_y / step);
		if (_type == MapType::AggregationMap)
		{
			if (p_col > this->_m_a_maxCol)
			{
				return rindex;
			}
			if (p_row > this->_m_a_maxRow)
			{
				return rindex;
			}
		}
		else
		{
			if (p_col > this->_m_s_maxCol)
			{
				return rindex;
			}
			if (p_row > this->_m_s_maxRow)
			{
				return rindex;
			}
		}
		rindex.first = p_col;
		rindex.second = p_row;
		return rindex;
	}

	void Map3D::map2AGrid()
	{
		auto bais_x = this->mWsPoint3.x() - this->mWsPoint1.x();
		auto bais_y = this->mWsPoint3.y() - this->mWsPoint1.y();

		//the max col and the max row
		this->_m_a_maxRow = ceil(bais_x / this->AgridStep);
		this->_m_a_maxCol = ceil(bais_y / this->AgridStep);

		//car1
		for (size_t i = 0; i < _m_a_maxCol; i++)
		{
			bgeo::DPoint3D pnt3D;
			pnt3D.set<Cartesian::X>(this->AgridStep * i + this->mWsPoint1.x() + this->AgridStep / 2);
			for (size_t j = 0; j < _m_a_maxRow; j++)
			{
				pnt3D.set<Cartesian::Y>(this->AgridStep * j + this->mWsPoint1.y() + this->AgridStep / 2);
				size_t pntType = VertType::WayVert;

				for (auto & it : this->_m_vDRing)
				{
					bgeo::DPoint pnt(pnt3D.get<Cartesian::X>(),pnt3D.get<Cartesian::Y>());
					auto _BWithIn = bg::within(pnt, it);
					if (_BWithIn)
					{
						pntType = VertType::Obstacle;
						goto insertPnt;
					}
				}


			insertPnt:

				GridIndex pntInd(i, j);
				double _z = this->getHeight(pnt3D.get<Cartesian::X>(), pnt3D.get<Cartesian::Y>());
				pnt3D.set<Cartesian::Z>(_z);
				PointVert3D pntVert3d;
				pntVert3d._pnt = pnt3D;
				pntVert3d._type = pntType;
				pntVert3d._pntInd = GridIndex(i, j);
				_m_AGridMap.insert(GridMapUnit(pntVert3d._pntInd, pntVert3d));				
			}
		}
	}

	void Map3D::grid2Graph(){
	for (size_t i=0 ; i < this->_m_vCrossAbi.size(); i++)
	{
		auto &crossAbi = this->_m_vCrossAbi[i];
		size_t graphInd = 0;
		bgeo::Graph _graph;
		map<GridIndex, int> map2graph;
		map<int, GridIndex> graph2map;

		for (auto & gridUnit : this->_m_AGridMap)
		{
			bgeo::VertexProperty _vertPro(gridUnit.second._pnt, gridUnit.second._type, gridUnit.first);
			//_vertPro._pnt = it.second._pnt;
			boost::add_vertex(_vertPro, _graph);
			map2graph.insert(std::pair<GridIndex, int>(gridUnit.first, graphInd));
			graph2map.insert(std::pair<int, GridIndex>(graphInd, gridUnit.first));
			graphInd++;
		}
		this->_m_vAllMap2Graph.push_back(map2graph);
		this->_m_vAllGraph2Map.push_back(graph2map);

		std::pair<bgeo::VertexIterator, bgeo::VertexIterator> vi = boost::vertices(_graph);

		for (bgeo::VertexIterator vit = vi.first; vit != vi.second; vit++)
		{
			bgeo::VertexDescriptor vd = *vit;
			if (_graph[vd]._type == VertType::WayVert)
			{
				auto localIndex = graph2map[vd];
				auto vlocalIndex = getSearchNeighbor(localIndex);
				vector<bgeo::VertexDescriptor> vvd;
				for (auto &it : vlocalIndex)
					vvd.push_back(map2graph[it]);
				
				double m_z = _graph[vd]._pnt.get<Cartesian::Z>();

				for(auto & e_vd : vvd)
				{ 
					auto &vp = _graph[e_vd];
					auto _z = vp._pnt.get<Cartesian::Z>();
					if (_z >= (m_z + crossAbi))
						continue;
					bgeo::EdgeProperty ep;
					ep.weight = bg::distance(_graph[vd]._pnt, _graph[e_vd]._pnt);
					boost::add_edge(vd, e_vd, ep, _graph);
				}
			}
		}

		auto num_edges = boost::num_edges(_graph);
		cout << "num_edges = " << num_edges << endl;
		this->_m_vAllGraph.push_back(_graph);
	}
	}

	std::vector<GridIndex> Map3D::getSearchNeighbor(GridIndex const & mindex)
	{

		//
		auto &grid = this->_m_AGridMap;
		std::vector<GridIndex> vIndex;
		
		for (auto i = mindex.first - 1; i <= (mindex.first + 1); i++)
		{
			for (auto j = mindex.second - 1; j <= (mindex.second + 1); j++)
			{
				if (grid.count(GridIndex(i, j)) == 1)
				{
					if ((i == mindex.first) && (j == mindex.second))
					{
					}
					else
					{
						if (grid[GridIndex(i, j)]._type == VertType::WayVert)
							vIndex.push_back(GridIndex(i, j));
					}
				}
			}
		}
		return vIndex;
	}

	double Map3D::getHeight(double const& _x, double const& _y) const
	{
		auto const bias_x = _x - this->mWsPoint1.x();
		auto const bias_y = _y - this->mWsPoint1.y();
		
		size_t const row = std::floor(bias_y / this->_m_originGridSize);
		size_t const col = std::floor(bias_x / this->_m_originGridSize);
		//cout << " row  = " << row << " col = " << col << endl;
		return this->_m_originData[row * this->_m_originColSize + col ];
	}



	PointVert3D::PointVert3D()
	{
	}

	PointVert3D::~PointVert3D()
	{
	}
	Map3D ex_mainMap;
}